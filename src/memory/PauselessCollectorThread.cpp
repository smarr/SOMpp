#include "PauselessCollectorThread.h"

#include <misc/defs.h>
#include "Worklist.h"
#include <interpreter/Interpreter.h>
#include <vm/Universe.h>
#include <vmobjects/VMMethod.h>
#include <vmobjects/VMObject.h>
#include <vmobjects/VMSymbol.h>
#include <vmobjects/VMFrame.h>
#include <vmobjects/VMBlock.h>
#include <vmobjects/VMPrimitive.h>
#include <vmobjects/VMClass.h>
#include <vmobjects/VMThread.h>
#include <vmobjects/VMEvaluationPrimitive.h>

/* TODO:
 1) Allow for more than 1 garbage collection thread (issue when requesting a checkpoint at this moment)
 2) Delete the stopped mutator threads at the end of each gc-cycle
 3) Create destructor for nonEmptyWorklists, blockedInterpreters, pagesToRelocate, pagesToUnblock
 4) make pagesToRelocate and PagesToUnblock one and the same thing
 5) make destructor for interpreter so that nonRelocatablePages are added to the fullpages again
    Mind you that this is dangarous to simply add them to the fullpages as there is no guarantee that all the pages are fully marked
    (no problem since the destructor is only called at the end of each gc cycle)
 6) provide a lock for the fullpages (at the moment it is "only" accessed when doing a root-set marking
    it is however also accessed when a threads'destructor is called
 7) Take amount of live data into account when doing relocation of pages
 8) Remove some unnecessary locks
 
 1) Clean up the new operator for VMObject / VMAbstractObject
 2) Change name of blockedMutex, ....
 3) Take a look at some of the things that are kept at the heap level but that perhaps should better be put at the collector level
 4) Ask Stefan about the VMThread join operation, seems to act really strange if I change it to the original setup
 */

#include <misc/debug.h>

#if GC_TYPE==PAUSELESS

#define REFERENCE_NMT_VALUE(REFERENCE) (((reinterpret_cast<intptr_t>(REFERENCE) & MASK_OBJECT_NMT) == 0) ? false : true)

int PauselessCollectorThread::numberOfGCThreads;
int PauselessCollectorThread::markValue;

pthread_mutex_t PauselessCollectorThread::blockedMutex;
pthread_mutex_t PauselessCollectorThread::markGlobalsMutex;
pthread_mutex_t PauselessCollectorThread::markRootSetsMutex;
pthread_mutex_t PauselessCollectorThread::markRootSetsCheckpointMutex;
pthread_cond_t  PauselessCollectorThread::doneMarkingRootSetsCondition;
pthread_cond_t  PauselessCollectorThread::blockedCondition;
int PauselessCollectorThread::numberRootSetsMarked;
int PauselessCollectorThread::numberRootSetsToBeMarked;
bool PauselessCollectorThread::doneSignalling;
bool PauselessCollectorThread::doneMarkingGlobals;
vector<Interpreter*>* PauselessCollectorThread::blockedInterpreters;

vector<Worklist*>* PauselessCollectorThread::nonEmptyWorklists;
pthread_mutex_t PauselessCollectorThread::nonEmptyWorklistsMutex;
pthread_mutexattr_t PauselessCollectorThread::attrNonEmptyWorklistsMutex;
pthread_cond_t  PauselessCollectorThread::waitingForWorkCondition;
pthread_cond_t  PauselessCollectorThread::waitingForCheckpointCondition;
int PauselessCollectorThread::numberOfGCThreadsDoneMarking;
int PauselessCollectorThread::numberOfMutatorsPassedSafepoint;
int PauselessCollectorThread::numberOfMutators;
bool PauselessCollectorThread::checkpointRequested;
bool PauselessCollectorThread::checkpointFinished;

bool PauselessCollectorThread::doneBlockingPages;
pthread_mutex_t PauselessCollectorThread::blockPagesMutex;
pthread_mutex_t PauselessCollectorThread::pagesToRelocateMutex;
pthread_cond_t PauselessCollectorThread::pagesToRelocateCondition;
vector<Page*>* PauselessCollectorThread::pagesToRelocate;
vector<Page*>* PauselessCollectorThread::pagesToUnblock;

int PauselessCollectorThread::numberOfGCThreadsFinished;
pthread_mutex_t PauselessCollectorThread::endOfCycleMutex;
pthread_cond_t PauselessCollectorThread::endOfCycleCond;
bool PauselessCollectorThread::endBool;

int PauselessCollectorThread::numberOfCycles;

void PauselessCollectorThread::InitializeCollector(int numberOfThreads) {
    numberOfGCThreads = numberOfThreads;
    markValue = 1;
    
    pthread_mutex_init(&blockedMutex, nullptr);
    pthread_mutex_init(&markGlobalsMutex, nullptr);
    pthread_mutex_init(&markRootSetsMutex, nullptr);
    pthread_mutex_init(&markRootSetsCheckpointMutex, nullptr);
    pthread_cond_init(&doneMarkingRootSetsCondition, nullptr);
    pthread_cond_init(&blockedCondition, nullptr);
    numberRootSetsMarked = 0;
    numberRootSetsToBeMarked = 0;
    doneSignalling = false;
    doneMarkingGlobals = false;
    blockedInterpreters = new vector<Interpreter*>();
    
    pthread_mutexattr_init(&attrNonEmptyWorklistsMutex);
    pthread_mutexattr_settype(&attrNonEmptyWorklistsMutex, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&nonEmptyWorklistsMutex, &attrNonEmptyWorklistsMutex);
    pthread_cond_init(&waitingForWorkCondition, nullptr);
    pthread_cond_init(&waitingForCheckpointCondition, nullptr);
    nonEmptyWorklists = new vector<Worklist*>();
    numberOfGCThreadsDoneMarking = 0;
    numberOfMutators = 0;
    checkpointRequested = false;
    checkpointFinished = false;
    
    doneBlockingPages = false;
    pthread_mutex_init(&blockPagesMutex, nullptr);
    pthread_mutex_init(&pagesToRelocateMutex, nullptr);
    pthread_cond_init(&pagesToRelocateCondition, nullptr);
    pagesToRelocate = new vector<Page*>();
    pagesToUnblock = new vector<Page*>();
    
    numberOfGCThreadsFinished = 0;
    pthread_mutex_init(&endOfCycleMutex, nullptr);
    pthread_cond_init(&endOfCycleCond, nullptr);
    endBool = false;
    
    numberOfCycles = 0;
}

static void mark_object(gc_oop_t* oop, Page*) {
    ReadBarrier(oop);
}

void PauselessCollectorThread::MarkObject(AbstractVMObject* obj) {
    if (obj->GetGCField() == markValue)
        return;
    PauselessHeap* const heap = GetHeap<PauselessHeap>();
    PauselessPage* page = heap->GetPageFromObj(obj);

    assert(REFERENCE_NMT_VALUE(obj) == false);
    assert(Universe::IsValidObject(obj));
    page->AddAmountLiveData(obj->GetObjectSize());
    obj->SetGCField(markValue);
    obj->WalkObjects(mark_object, nullptr);
}

void PauselessCollectorThread::AddBlockedInterpreter(Interpreter* interpreter) {
    pthread_mutex_lock(&blockedMutex);
    blockedInterpreters->push_back(interpreter);
    pthread_cond_signal(&blockedCondition);
    pthread_mutex_unlock(&blockedMutex);
}

void PauselessCollectorThread::SignalRootSetMarked() {
    pthread_mutex_lock(&markRootSetsCheckpointMutex);
    numberRootSetsMarked++;
    pthread_cond_signal(&doneMarkingRootSetsCondition);
    pthread_mutex_unlock(&markRootSetsCheckpointMutex);
}

void PauselessCollectorThread::AddNonEmptyWorklist(Worklist* worklist) {
    pthread_mutex_lock(&nonEmptyWorklistsMutex);
    if (!checkpointFinished) {
        nonEmptyWorklists->push_back(worklist);
        if (checkpointRequested) {
            checkpointRequested = false;
            pthread_cond_signal(&waitingForCheckpointCondition);
        } else {
            pthread_cond_signal(&waitingForWorkCondition);
        }
    }
    pthread_mutex_unlock(&nonEmptyWorklistsMutex);
}

void PauselessCollectorThread::SignalSafepointReached(bool* safePointRequested) {
    pthread_mutex_lock(&nonEmptyWorklistsMutex);
    numberOfMutatorsPassedSafepoint++;
    *safePointRequested = false;
    pthread_cond_signal(&waitingForCheckpointCondition);
    pthread_mutex_unlock(&nonEmptyWorklistsMutex);
}

void PauselessCollectorThread::AddGCWork(AbstractVMObject* work) {
    worklist.AddWorkGC(work);
}

/*pthread_mutex_t* PauselessCollectorThread::GetMarkRootSetMutex() {
    return &signalRootSetMarkingMutex;
}

pthread_mutex_t* PauselessCollectorThread::GetBlockPagesMutex() {
    return &SignalGCTrapMutex;
} */

int PauselessCollectorThread::GetCycle() {
    return numberOfCycles;
}

int PauselessCollectorThread::GetMarkValue() {
    return markValue;
}

PauselessCollectorThread::PauselessCollectorThread() : BaseThread(nullptr) {
    GetUniverse()->RegisterGCThreadInThreadLocal(this);
}

#include <unistd.h>

void PauselessCollectorThread::Collect() {
#warning there seems to be a bug lurking here, I saw the very first iteration \
of the GC doing working on the very same page the mutator thread was working\
with, this should not happen, the mutator should need to yield that page first\
or, the other way around, the GC should only consider yielded pages. \
I don't remember well, but, I think, I saw the GC relocating stuff on that page.

#warning hack:
    sleep(10);
    
    
    while (true) {
        
        expectedNMT = !expectedNMT;
        PauselessHeap* const test = GetHeap<PauselessHeap>();
        
        GetUniverse()->ErrorPrint("[GC] Start RootSet Marking");
        //------------------------
        // ROOT-SET MARKING
        //------------------------
        if (!doneSignalling && pthread_mutex_trylock(&markRootSetsMutex) == 0) {
            numberOfCycles++;
            //GetUniverse()->ErrorPrint("[GC] " + to_string(numberOfCycles));
            
            {
#warning TODO: we need to make sure in this block, that no new interpreters are going to join, because then, we might have an inconsistent rootset? don't know exactly what the problem is. But, all rootsets need to be marked.
                unique_ptr<vector<Interpreter*>> interpreters = GetUniverse()->GetInterpretersCopy();
                numberRootSetsToBeMarked = interpreters->size();
                numberRootSetsMarked = 0;
                for (vector<Interpreter*>::iterator it = interpreters->begin() ; it != interpreters->end(); ++it) {
                    (*it)->TriggerMarkRootSet();
                }
            }
            
            pthread_mutex_lock(&blockedMutex);
            doneSignalling = true;
            pthread_mutex_unlock(&blockedMutex);
            pthread_cond_broadcast(&blockedCondition);
            pthread_mutex_unlock(&markRootSetsMutex);
        }
        
        // one gc thread marks the globals
        if (!doneMarkingGlobals && pthread_mutex_trylock(&markGlobalsMutex) == 0) {
            GetUniverse()->WalkGlobals(mark_object, nullptr);
            doneMarkingGlobals = true;
            pthread_mutex_unlock(&markGlobalsMutex);
        }
    
        // gc threads mark the root sets of interpreters unable to do so because they are blocked
        while (true) {
            pthread_mutex_lock(&blockedMutex);
            while (!doneSignalling && blockedInterpreters->empty()) {
                pthread_cond_wait(&blockedCondition, &blockedMutex);
            }
            if (!blockedInterpreters->empty()) {
                Interpreter* blockedInterpreter = blockedInterpreters->back();
                blockedInterpreters->pop_back();
                pthread_mutex_unlock(&blockedMutex);
                blockedInterpreter->MarkRootSetByGC();
            } else {
                pthread_mutex_unlock(&blockedMutex);
                break;
            }
        }
    
        // barrier that makes sure that all root-sets have been marked before continuing
        pthread_mutex_lock(&markRootSetsCheckpointMutex);
        while (numberRootSetsMarked != numberRootSetsToBeMarked && !doneMarkingGlobals) {
            pthread_cond_wait(&doneMarkingRootSetsCondition, &markRootSetsCheckpointMutex);
        }
        pthread_cond_broadcast(&doneMarkingRootSetsCondition);
        pthread_mutex_unlock(&markRootSetsCheckpointMutex);
        
        //GetUniverse()->ErrorPrint("[GC] Start Marking Phase");
        
        //------------------------
        // CONTINUE MARKING PHASE
        //------------------------
        
        while (true) {
            
            while (!worklist.Empty()) {
                AbstractVMObject* obj = worklist.GetWork();
                MarkObject(obj);
            }
            
            //try to take work from interpreter worklists with work
            pthread_mutex_lock(&nonEmptyWorklistsMutex);
            numberOfGCThreadsDoneMarking++;
            while (nonEmptyWorklists->empty() && numberOfGCThreadsDoneMarking != numberOfGCThreads) {
                pthread_cond_wait(&waitingForWorkCondition, &nonEmptyWorklistsMutex);
            }
            if (!nonEmptyWorklists->empty()) {
                nonEmptyWorklists->back()->MoveWork(&worklist);
                nonEmptyWorklists->pop_back();
                numberOfGCThreadsDoneMarking--;
                pthread_mutex_unlock(&nonEmptyWorklistsMutex);
            } else {
                if (checkpointFinished) {
                    pthread_mutex_unlock(&nonEmptyWorklistsMutex);
                    break;
                }
                numberOfGCThreadsDoneMarking--;
                numberOfMutatorsPassedSafepoint = 0;
                unique_ptr<vector<Interpreter*>> interpreters = GetUniverse()->GetInterpretersCopy();
                for (vector<Interpreter*>::iterator it = interpreters->begin() ; it != interpreters->end(); ++it) {
                    (*it)->RequestSafePoint();
                }
                numberOfMutators = interpreters->size();
                checkpointRequested = true;
                // after this, this thread should also wait to see whether or not all threads succesfully pass the safepoint
                while (numberOfMutators != numberOfMutatorsPassedSafepoint && nonEmptyWorklists->empty()) {
                    pthread_cond_wait(&waitingForCheckpointCondition, &nonEmptyWorklistsMutex);
                }
                if (numberOfMutators == numberOfMutatorsPassedSafepoint && checkpointRequested) {
                    checkpointFinished = true;
                    checkpointRequested = false;
                    numberOfGCThreadsDoneMarking++;
                    pthread_cond_broadcast(&waitingForWorkCondition);
                    pthread_mutex_unlock(&nonEmptyWorklistsMutex);
                    break;
                } else if (!nonEmptyWorklists->empty()) {
                    nonEmptyWorklists->back()->MoveWork(&worklist);
                    nonEmptyWorklists->pop_back();
                    //numberOfGCThreadsDoneMarking--;
                    pthread_mutex_unlock(&nonEmptyWorklistsMutex);
                }
                checkpointFinished = false;
                pthread_mutex_unlock(&nonEmptyWorklistsMutex);
            }
        }

        //------------------------
        // RELOCATE PHASE
        //------------------------
        if (!doneBlockingPages && pthread_mutex_trylock(&blockPagesMutex) == 0) {
            
            {
#warning in this block, we usually made sure that now new interpreters are joining, to be sure that all traps are correctly set. \
 we might be able to solve that differently, first the GC thread writes the expected value to a public location (synchronized),  \
 and then, it starts flipping the other bits
                // disable GC-trap from running interpreters
                unique_ptr<vector<Interpreter*>> interpreters = GetUniverse()->GetInterpretersCopy();
                for (vector<Interpreter*>::iterator it = interpreters->begin() ; it != interpreters->end(); ++it) {
                    (*it)->DisableGCTrap();
                }
            }
            
            GetHeap<HEAP_CLS>()->numberOfMutatorsWithEnabledGCTrap = 0;
            // unblock pages that were blocked during the previous cycle
            for (PauselessPage* page : *pagesToUnblock) {
                page->UnBlock();
                GetHeap<PauselessHeap>()->pagedHeap.ReturnFreePage(page);
            }
            pagesToUnblock->clear();
            // block pages that will be subject for relocation
                                //pthread_mutex_lock();
            vector<PauselessPage*>& fullPages = GetHeap<PauselessHeap>()->
                                                    pagedHeap.GetFullPages_alreadyLocked();
            for (PauselessPage* page : fullPages) {
                page->Block();
                pagesToRelocate->push_back(page);
                pagesToUnblock->push_back(page);
            }
            fullPages.clear();

            {

#warning some as above: in this block, we usually made sure that now new interpreters are joining, to be sure that all traps are correctly set. \
we might be able to solve that differently, first the GC thread writes the expected value to a public location (synchronized),  \
and then, it starts flipping the other bits
                // enable the GC-trap again
                auto interpreters = GetUniverse()->GetInterpretersCopy();
                GetHeap<HEAP_CLS>()->numberOfMutatorsNeedEnableGCTrap = interpreters->size();
                assert(GetHeap<HEAP_CLS>()->numberOfMutatorsWithEnabledGCTrap == 0);
                for (auto interp : *interpreters) {
                    interp->SignalEnableGCTrap();
                }
            }
            
            // All gc-threads need to wait till all mutators have the gc-trap enabled before starting relocation
            pthread_mutex_lock(&(GetHeap<HEAP_CLS>()->gcTrapEnabledMutex));
            while (GetHeap<HEAP_CLS>()->numberOfMutatorsNeedEnableGCTrap != GetHeap<HEAP_CLS>()->numberOfMutatorsWithEnabledGCTrap) {
                pthread_cond_wait(&(GetHeap<HEAP_CLS>()->gcTrapEnabledCondition), &(GetHeap<HEAP_CLS>()->gcTrapEnabledMutex));
            }
            pthread_mutex_unlock(&(GetHeap<HEAP_CLS>()->gcTrapEnabledMutex));
            
            
            // signal the other gc-threads that they may start relocating
            doneBlockingPages = true;
            pthread_cond_broadcast(&pagesToRelocateCondition);
            pthread_mutex_unlock(&blockPagesMutex);
        } else {
            pthread_mutex_lock(&pagesToRelocateMutex);
            while (!doneBlockingPages)
                pthread_cond_wait(&pagesToRelocateCondition, &pagesToRelocateMutex);
            pthread_mutex_unlock(&pagesToRelocateMutex);
        }
        
        while (true) {
            pthread_mutex_lock(&pagesToRelocateMutex);
            if (!pagesToRelocate->empty()) {
                Page* fromPage = pagesToRelocate->back();
                pagesToRelocate->pop_back();
                pthread_mutex_unlock(&pagesToRelocateMutex);
                fromPage->RelocatePage();
            } else {
                pthread_mutex_unlock(&pagesToRelocateMutex);
                break;
            }
        }
        
        //wait for all gc-threads to finish before starting a new cycle
        pthread_mutex_lock(&endOfCycleMutex);
        numberOfGCThreadsFinished++;
        if (endBool)
            endBool = false;
        if (numberOfGCThreadsFinished == numberOfGCThreads) {
            numberOfGCThreadsDoneMarking = 0;
            doneSignalling = false;
            doneBlockingPages = false;
            doneMarkingGlobals = false;
            checkpointFinished = false;
            markValue++;
            numberOfGCThreadsFinished = 0;
            endBool = true;
            //std::cout << "cycle finished: " << numberOfCycles << std::endl;
        }
        while (numberOfGCThreads != numberOfGCThreadsFinished && !endBool) {
            pthread_cond_wait(&endOfCycleCond, &endOfCycleMutex);
        }
        pthread_cond_broadcast(&endOfCycleCond);
        pthread_mutex_unlock(&endOfCycleMutex);
        
        //doneSignalling = false;
        //doneBlockingPages = false;
        //doneMarkingGlobals = false;
        //numberOfGCThreadsDoneMarking = 0;
        //markValue++;
        //checkpointFinished = false;
        //numberOfGCThreadsFinished = 0;
        
        
        GetUniverse()->ErrorPrint("[GC] End of cycle");
        GetUniverse()->ErrorPrint("=================");
        
        
    } //end of cycle

}

// FOR DEBUGGING PURPOSES
void PauselessCollectorThread::CheckMarkingOfObject(vm_oop_t oop) {
    if (IS_TAGGED(oop))
        return;
    
    AbstractVMObject* obj = static_cast<AbstractVMObject*>(oop);
    
    assert(Universe::IsValidObject(obj));
    if (obj->GetGCField2() == markValue)
        return;
    obj->SetGCField2(markValue);
    PauselessHeap* const heap = GetHeap<PauselessHeap>();
    PauselessPage* page = heap->GetPageFromObj(obj);
    
    if (heap->pagedHeap.IsInFullPages(page)) {
        assert(obj->GetGCField() == markValue);
        obj->CheckMarking(CheckMarkingOfObject);
    }
}

void PauselessCollectorThread::CheckMarking() {
    GetUniverse()->CheckMarkingGlobals(CheckMarkingOfObject);
    unique_ptr<vector<Interpreter*>> interpreters = GetUniverse()->GetInterpretersCopy();
    for (vector<Interpreter*>::iterator it = interpreters->begin() ; it != interpreters->end(); ++it) {
        (*it)->CheckMarking(CheckMarkingOfObject);
    }
}

#endif
