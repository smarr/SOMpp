#include "PauselessCollector.h"

#include "../../misc/defs.h"

#include "Worklist.h"
#include "../PagedHeap.h"
#include "../../interpreter/Interpreter.h"
#include "../../vm/Universe.h"
#include "../../vmobjects/VMMethod.h"
#include "../../vmobjects/VMObject.h"
#include "../../vmobjects/VMSymbol.h"
#include "../../vmobjects/VMFrame.h"
#include "../../vmobjects/VMBlock.h"
#include "../../vmobjects/VMPrimitive.h"
#include "../../vmobjects/VMClass.h"
#include "../../natives/VMThread.h"
#include "../../vmobjects/VMEvaluationPrimitive.h"

#if GC_TYPE==PAUSELESS

int PauselessCollector::numberOfGCThreads;
volatile int PauselessCollector::numberOfGCThreadsDoneMarking;

volatile bool PauselessCollector::doneSignalling;
volatile bool PauselessCollector::doneMarkingGlobals;
volatile bool PauselessCollector::doneBlockingPages;

pthread_mutex_t PauselessCollector::blockedMutex;//
pthread_mutex_t PauselessCollector::markGlobalsMutex;//
pthread_mutex_t PauselessCollector::markRootSetsMutex;//
pthread_mutex_t PauselessCollector::leftoverRootSetMutex;//
pthread_mutex_t PauselessCollector::doneMarkingMutex;//
pthread_mutex_t PauselessCollector::blockPagesMutex;//

pthread_cond_t PauselessCollector::blockedCondition;//
pthread_cond_t PauselessCollector::doneMarkingRootsCondition;//
pthread_cond_t PauselessCollector::doneMarkingCondition;//

vector<Interpreter*> PauselessCollector::blockedInterpreters;
vector<Interpreter*> PauselessCollector::leftoverInterpretersRootSetBarrier;
vector<Interpreter*> PauselessCollector::leftoverInterpretersMarkingBarrier;

void PauselessCollector::MarkObject(VMOBJECT_PTR obj, Worklist* worklist) {
    //still need to add code so that the marked object it's size is taken into account
    assert(Universe::IsValidObject(obj));
#ifdef USE_TAGGING
    //don't process tagged objects
    if (IS_TAGGED(obj))
        return;
#endif
    if (obj->GetGCField() & MASK_OBJECT_IS_MARKED)
        return;
    
    //Page* page = allPages->at(((size_t)obj - (size_t)memoryStart) / PAGE_SIZE);
    //page->AddAmountLiveData(obj->GetObjectSize());
    
    obj->SetGCField(MASK_OBJECT_IS_MARKED);
    obj->MarkReferences(worklist);
}


PauselessCollector::PauselessCollector(PagedHeap* heap, int numberOfGCThreads) : GarbageCollector(heap) {
    pthread_mutex_init(&blockedMutex, NULL);
    pthread_mutex_init(&markGlobalsMutex, NULL);
    pthread_mutex_init(&markRootSetsMutex, NULL);
    pthread_mutex_init(&leftoverRootSetMutex, NULL);
    pthread_mutex_init(&doneMarkingMutex, NULL);
    pthread_mutex_init(&blockPagesMutex, NULL);
    pthread_cond_init(&blockedCondition, NULL);
    pthread_cond_init(&doneMarkingRootsCondition, NULL);
    pthread_cond_init(&doneMarkingCondition, NULL);
    
    this->numberOfGCThreads = numberOfGCThreads;
    numberOfGCThreadsDoneMarking = 0;
    
    doneMarkingGlobals = false;
    doneSignalling = false;
    doneBlockingPages = false;
    
    /*for (int i=0; i < numberOfGCThreads; i++) {
        ThreadId tid = 0;
        pthread_create(&tid, NULL, &GCThread, NULL);
    } */
}

void PauselessCollector::AddBlockedInterpreter(Interpreter* interpreter) {
    pthread_mutex_lock(&blockedMutex);
    blockedInterpreters.push_back(interpreter);
    pthread_cond_signal(&blockedCondition);
    pthread_mutex_unlock(&blockedMutex);
}

// This method is part of a barrier that ensures that the gc-cycle does not continue till all root-sets have been marked
// could be improved in that when we are not during the correct phase of the cycle this should do nothing
void PauselessCollector::SignalRootSetBarrier(Interpreter* interpreter) {
    pthread_mutex_lock(&leftoverRootSetMutex);
    leftoverInterpretersRootSetBarrier.erase(std::remove(leftoverInterpretersRootSetBarrier.begin(), leftoverInterpretersRootSetBarrier.end(), interpreter), leftoverInterpretersRootSetBarrier.end());
    if (leftoverInterpretersRootSetBarrier.empty()) {
        //I could here add the possibility to clean all the interpreters of all the dead ones
        pthread_cond_broadcast(&doneMarkingRootsCondition);
    }
    pthread_mutex_lock(&leftoverRootSetMutex);
}

void* PauselessCollector::GCThread(void*) {
    
    Worklist* localWorklist = new Worklist();
    vector<Interpreter*>* interpreters;
    
    //------------------------
    // ROOT-SET MARKING
    //------------------------
    //doneMarkingGlobals = false; // I still need to change this
    //doneSignalling = false;
    
    // one thread signals to all mutator threads to mark their root-sets
    // interpreters which are blocked are added to a vector containing pointers to these interpreters so that there root-set can be processed by a gc thread
    if (!doneSignalling && pthread_mutex_trylock(&markRootSetsMutex) == 0) {
        interpreters = _UNIVERSE->GetInterpretersCopy();
        leftoverInterpretersRootSetBarrier = vector<Interpreter*>(interpreters->begin(),interpreters->end());
        for (vector<Interpreter*>::iterator it = interpreters->begin() ; it != interpreters->end(); ++it) {
            (*it)->TriggerMarkRootSet();
        }
        pthread_mutex_lock(&blockedMutex);
        doneSignalling = true;
        pthread_mutex_unlock(&blockedMutex);
        pthread_cond_broadcast(&blockedCondition);
        pthread_mutex_unlock(&markRootSetsMutex);
        delete interpreters;
    }
    
    // one gc thread marks the globals
    if (!doneMarkingGlobals && pthread_mutex_trylock(&markGlobalsMutex) == 0) {
        _UNIVERSE->MarkGlobals(localWorklist);
        doneMarkingGlobals = true;
        pthread_mutex_unlock(&markGlobalsMutex);
    }
    
    // gc threads mark the root sets of interpreters unable to do so because they are blocked
    while (true) {
        pthread_mutex_lock(&blockedMutex);
        while (!doneSignalling && blockedInterpreters.empty()) {
            pthread_cond_wait(&blockedCondition, &blockedMutex);
        }
        if (!blockedInterpreters.empty()) {
            Interpreter* blockedInterpreter = blockedInterpreters.back();
            blockedInterpreters.pop_back();
            pthread_mutex_unlock(&blockedMutex);
            blockedInterpreter->MarkRootSet(); //this will also do RemoveLeftoverInterpreter, same for dummy cases
        } else {
            pthread_mutex_unlock(&blockedMutex);
            break;
        }
    }
    
    // barrier that makes sure that all root-sets have been marked before continuing
    pthread_mutex_lock(&leftoverRootSetMutex);
    while (!leftoverInterpretersRootSetBarrier.empty()) {
        pthread_cond_wait(&doneMarkingRootsCondition, &leftoverRootSetMutex);
    }
    pthread_mutex_unlock(&leftoverRootSetMutex);
    
    //------------------------
    // CONTINUE MARKING PHASE
    //------------------------
    while (true) {
        
        do {
            // if local worklist empty, try to take work
            interpreters = _UNIVERSE->GetInterpretersCopy(); //this could be potentially very costly?
            vector<Interpreter*>::iterator it = interpreters->begin();
            while (localWorklist->Empty() && it != interpreters->end()) {
                (*it)->MoveWork(localWorklist);
                it++;
            }
            // after a pass over the mutator worklists, if their is work on the local worklist, perform marking of the work on the local worklist
            while (!localWorklist->Empty()) {
                VMOBJECT_PTR obj = localWorklist->GetWork();
                MarkObject(obj, localWorklist);
            }
        } while (numberOfNonEmptyMutatorWorklists != 0);
        
        
        
        
        
        // the gc-thread has run out of work to do and was unable to take work from a mutator worklist as these are, for the moment, empty
        pthread_mutex_lock(&doneMarkingMutex);
        numberOfGCThreadsDoneMarking++;
        // first part of barrier is to wait till all gc-threads are in the situation of having no more work
        while (numberOfGCThreadsDoneMarking != numberOfGCThreads && numberOfNonEmptyMutatorWorklists == 0) {
            // condition can be triggered by NMT-traps, other GC-threads, or mutators reaching safepoint
            pthread_cond_wait(&doneMarkingCondition , &doneMarkingMutex);
        }
        if (numberOfNonEmptyMutatorWorklists != 0) {
            // there is potentially some work to take -> restart while loop
            numberOfGCThreadsDoneMarking--;
            pthread_mutex_unlock(&doneMarkingMutex);
        } else {
            // wait for safepoint passes
            while ( NUMBER_OF_MUTATORS == NUMBER_OF_MUTATORS_PASSED_CHECKPOINT && numberOfNonEmptyMutatorWorklists == 0) {
                pthread_cond_wait(&doneMarkingCondition, &doneMarkingMutex);
            }
            if (numberOfNonEmptyMutatorWorklists != 0) {
                // there is potentially some work to take -> restart while loop
                numberOfGCThreadsDoneMarking--;
                pthread_mutex_unlock(&doneMarkingMutex);
            } else {
                // the gc-threads may continue there work
                pthread_cond_signal(&doneMarkingCondition);
                pthread_mutex_lock(&doneMarkingMutex);
                break;
                
                // I can set a variable here, that when set to true allows the other gc-threads to immediately break from the while
                // this to ensure that subsequent traps (that are part of the next cycle) do not interfer with this cycle
                
            }
        }
        
    }

    
    //------------------------
    // RELOCATE PHASE
    //------------------------
    //doneBlockingPages = false; // I still need to change this
    
    // find pages to relocate, and mark them blocked
    // signal to the mutators that, after reaching a safepoint, their gc-trap may trigger
    if (!doneBlockingPages && pthread_mutex_trylock(&blockPagesMutex) == 0) {
        // make sure that all gc-threads have their gc-trap disabled
        interpreters = _UNIVERSE->GetInterpretersCopy();
        for (vector<Interpreter*>::iterator it = interpreters->begin() ; it != interpreters->end(); ++it) {
            (*it)->DisableGCTrap();
        }
        //mark all pages that should be relocated and add them to a list of pages that need to have their objects relocated
        for (std::vector<Page*>::iterator page = allPages->begin(); page != allPages->end(); ++page) {
            if ((*page)->ThresholdReached()) {
                (*page)->Block();
                ptread_mutex_lock(&blockedMutex);
                pagesToRelocate.push_back(*page);
                pthread_cond_signal(&blockedCondition);
                pthread_mutex_unlock(&blockedMutex);
            }
        }
        // enable GC-trap again
        for (it = interpreters.begin(); it != interpreters.end(); ++it) {
            it->SignalEnableGCTrap();
        }
        
    }
    
    Page* page = _HEAP->RequestPage();
    
    while (true) {
        pthread_mutex_lock(&blockedMutex);
        while (!doneBlockingPages && pagesToRelocate.empty()) {
            pthread_cond_wait(&blockedCondition, &blockedMutex);
        }
        if (!pagesToRelocate.empty()) {
            take page
            pthread_mutex_unlock();
            do the work;
        } else {
            pthread_mutex_unlock();
            break;
        }
    }

    
}
    
 /*
    
    
    if (gcThreadId == MASTER_GC_THREAD) {
        lock vector
        for each Interpreter in to_be_delete_vector
            do delte
        unlock vector
    }
    
    
    
    pthread_exit(NULL);
}

     */


#endif


















 /*
 if ((*it)->Blocked()) {
 pthread_mutex_lock(&blockedMutex);
 blockedInterpreters.push_back(*it);
 pthread_cond_signal(&blockedCondition);
 pthread_mutex_unlock(&blockedMutex);
 }
 else
 (*it)->TriggerMarkRootSet();
 */


// This method is part of a barrier that ensures that the gc-cycle does not continue till all marking has been done
/*void PauselessCollector::RemoveLeftoverInterpreterMarkingBarrier(Interpreter* interpreter) {
 pthread_mutex_lock(&doneMarkingMutex);
 leftoverInterpretersRootSetBarrier.erase(std::remove(leftoverInterpretersMarkingBarrier.begin(), leftoverInterpretersMarkingBarrier.end(), interpreter), leftoverInterpretersMarkingBarrier.end());
 pthread_cond_signal(&doneMarkingCondition);
 pthread_mutex_lock(&doneMarkingMutex);
 } */






/*
 
 
 //std::sort(interpreters.begin(), interpreters.end(), compByInterpreterId); //dit sorteren kan direct en 1malig gebeuren
 
 pthread_mutex_lock(interpretersWithMarkedRootSetsMutex);
 interpretersWithMarkedRootSets = _UNIVERSE->get.....;
 std::sort(_UNIVERSE->interpretersWithMarkedRootSets.begin(), _UNIVERSE->interpretersWithMarkedRootSets.begin(), compByInterpreterId);
 
 
 while (interpreters !SUBSET_OF interpreters with root-sets marked) {
 pthread_cond_wait(&barrier, &interpretersWithMarkedRootSetsMutex);
 }
 //pthread_mutex_unlock(&threadCountMutex);
 
 */
        
        
        
        /*
         
         // gc threads mark the root sets of interpreters unable to do so because they are blocked
         while (!doneSignaling || !blockedInterpreters.empty()) {
         //as long as the gc-thread that signals all mutator threads to mark their root-sets is not finished signalling do the while loop
         pthread_mutex_lock(&blockedMutex);
         while (!doneSignalling && blockedInterpreters.empty()) {
         pthread_cond_wait(&blockedCondition, &blockedMutex);
         }
         if (!doneSignalling) {
         //the additional check is to see whether we got here because doneSignalling has become true or because there is genuinely a blocked interpreter to process
         Interpreter* blockedInterpreter = blockedInterpreters.back();
         blockedInterpreters.pop_back();
         pthread_mutex_unlock(&blockedMutex);
         blockedInterpreter->markRootSet();
         }
         }
         
         */

/*

 
 vector<Interpreter*>::iterator it = interpreters->begin();
 while (true) {
 //still needs a finished stop-condition
 while (!localWorklist->Empty()) {
 // as long as there is work on the gc-thread local worklist take work from here
 VMOBJECT_PTR obj = localWorklist->PopFront();
 MarkObject(obj, localWorklist);
 }
 do {
 // when the local worklist runs out try to take work from the worklist of an interpreter
 // getGCWork makes use of mutex_trylock in order to prevent overhead
 it->getGCWork(localWorklist);
 it++;
 } while (localWorklist->Empty() && it != interpreters->end);
 it = interpreters->begin();
 }
 
 
*/



    
    
    
    
    
    
    
    
    
/*

 
 
 
 
 
 
 
 
 
 pthread_mutex_lock(&endMarkPhaseMutex);
 numberOfGCThreadsDoneMarking++;
 while (numberOfGCThreadsDoneMarking != numberOfGCThreads) {
 pthread_cond_wait();
 while () {
 
 }
 }
 
 
 
 
 pthread_mutex_lock(&endMarkPhaseMutex);
 numberOfGCThreadsDoneMarking++; //this is something that needs to be reset after the cycle
 while (numberOfGCThreadsDoneMarking != numberOfGCThreads && numberOfNonEmptyWorklists == 0) {
 pthread_cond_wait(&endMarkPhaseCondition, &endMarkPhaseMutex);
 //it can wake up because a NMT-bit was set which triggers a pthread_cond_signal
 //because all gc-threads have reached this fase
 }
 if (numberOfNonEmptyWorklists != 0) {
 marking()
 }
 pthread_cond_signal(&endMarkPhaseCondition);
 pthread_mutex_unlock(&endMarkPhaseCondition);
 
 
 
 
 
 
 
 
 
 if (numberOfNonEmptyWorklists != 0) {
 //to differeniate between waking up because marking is done, or waking up because a NMT-bit was flipped
 numberOfGCThreadsDoneMarking--;
 pthread_mutex_unlock(&endMarkPhaseMutex);
 marking();
 pthread_mutex_lock(&endMarkPhaseMutex);
 }
 
 
 
 
 }
 pthread_cond_signal(&endMarkPhaseCondition);
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 //somewhere in the NMT-trap there is something like:
 pthread_cond_signal(&endMarkPhaseCondition);
 
 
 
 
 
 
 //probleem met dat ding hier is dat ik er niet uit kan jumpen indien nodig....
 
 
 
 
 
 
 
 while (numberOfGCThreadsDoneMarking != numberOfGCThreads) {
 pthread_cond_wait(&endMarkPhaseCondition, &endMarkPhaseMutex);
 if (NMT-triggered) {
 numberOfGCThreadsDoneMarking--;
 pthread_mutex_unlock();
 marking();
 }
 pthread_mutex_lock()
 numberOfGCThreadsDoneMarking++;
 }
 pthread_cond_broadcast(&endMarkPhaseCondition); //can I also do this with signal? because a chain of signals will take place
 
 // barrier that makes sure that all mutator threads cross the checkpoint
 pthread_mutex_lock(&checkpoint);
 while (mutators having reached checkpoint != interpreters.Size()) {
 pthread_cond_wait(, &checkpoint);
 if (NMT-triggered) {
 number
 }
 }
 pthread_mutex_
 
 
 
 
 
 
 
 
 
 
 //barrier
 pthread_mutex_lock(&endMarkPhaseMutex);
 numberOfGCThreadsDoneMarking++;
 while (numberOfGCThreadsDoneMarking != numberOfGCThreads &&
 mutators having reached checkpoint != interpreters.Size()) {
 
 pthread_cond_wait(&endMarkPhaseCondition, &endMarkPhaseMutex);
 
 if (numberOfNonEmptyWorklists != 0) {
 numberOfGCThreadsDoneMarking--;
 marking();
 }
 
 }
 pthread_mutex_unlock(&endMarkPhaseMutex);
 
 
 
 //wake-up needs to happen in two situations
 // either he has to perform some more work
 // or else it is because the relocation phase may start
 
 
 //------------------------
 // RELOCATE PHASE
 //------------------------
 
 // find pages to relocate, and mark them blocked
 if (!doneBlockingPages && pthread_mutex_trylock(&blockPagesMutex) == 0) {
 //mark all pages that should be relocated and add them to a list of pages that need to have their objects relocated
 for (std::vector<Page*>::iterator page = allPages->begin(); page != allPages->end(); ++page) {
 if ((*page)->ThresholdReached()) {
 (*page)->Block();
 pagesToRelocate.push_back(*page);
 }
 }
 for (it = interpreters.begin(); it != interpreters.end(); ++it) {
 it->SignalEnablingGCTrap(); => when this is triggered the mutator should also look to take another page
 //still need to make sure that blocked threads also perform the necessary action -> for when they become unblocked -> perform check when getting out of blocked status
 }
 }
 
 while (!doneBlockingPages) {
 
 }
 
 
 pthread_mutex_wait;
 
 
 
 // gc threads mark the root sets of interpreters unable to do so because they are blocked
 while (!doneSignaling) {
 //as long as the gc-thread that signals all mutator threads to mark their root-sets is not finished signalling do the while loop
 pthread_mutex_lock(&blockedMutex);
 while (!doneSignalling && blockedInterpreters.empty()) {
 pthread_cond_wait(&blockedCondition, &blockedMutex);
 }
 if (!doneSignalling) {
 //the additional check is to see whether we got here because doneSignalling has become true or because there is genuinely a blocked interpreter to process
 Interpreter* blockedInterpreter = blockedInterpreters.back();
 blockedInterpreters.pop_back();
 pthread_mutex_unlock(&blockedMutex);
 blockedInterpreter->markRootSet();
 }
 }
 
 Page* pageToRelocateTo = _HEAP->RequestPage(); //change this to allow any page
 
 
 
 
*/