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

pthread_mutex_t PauselessCollector::blockedMutex;
pthread_mutex_t PauselessCollector::markGlobalsMutex;
pthread_mutex_t PauselessCollector::markRootSetsMutex;
pthread_mutex_t PauselessCollector::markRootSetsCheckpointMutex;
pthread_cond_t  PauselessCollector::doneMarkingRootSetsCondition;
pthread_cond_t  PauselessCollector::blockedCondition;
int PauselessCollector::numberRootSetsMarked;
int PauselessCollector::numberRootSetsToBeMarked;
bool PauselessCollector::doneSignalling;
bool PauselessCollector::doneMarkingGlobals;
vector<Interpreter*>* PauselessCollector::blockedInterpreters;

pthread_mutex_t PauselessCollector::nonEmptyWorklistsMutex;
pthread_cond_t  PauselessCollector::waitingForWorkCondition;
vector<Worklist*>* PauselessCollector::nonEmptyWorklists;
bool PauselessCollector::doneRequestCheckpoint;
int PauselessCollector::numberOfGCThreadsDoneMarking;
atomic<int> PauselessCollector::numberOfMutatorsPassedSafepoint;
int PauselessCollector::numberOfMutators;

bool PauselessCollector::doneBlockingPages;
pthread_mutex_t PauselessCollector::blockPagesMutex;
pthread_mutex_t PauselessCollector::pagesToRelocateMutex;
pthread_cond_t PauselessCollector::pagesToRelocateCondition;
vector<Page*>* PauselessCollector::pagesToRelocate;

void PauselessCollector::MarkObject(vm_oop_t oop, Worklist* worklist) {
#warning still need to add code so that the marked object it's size is taken into account
    
    //don't process tagged objects
    if (IS_TAGGED(oop))
        return;
    
    AbstractVMObject* obj = static_cast<AbstractVMObject*>(oop);
    assert(Universe::IsValidObject(obj));

    if (obj->GetGCField() & MASK_OBJECT_IS_MARKED)
        return;
    
    Page* page = _HEAP->allPages->at(((size_t)obj - (size_t)_HEAP->memoryStart) / PAGE_SIZE);
    page->AddAmountLiveData(obj->GetObjectSize());
    
    obj->SetGCField(MASK_OBJECT_IS_MARKED);
    obj->MarkReferences();
}


PauselessCollector::PauselessCollector(PagedHeap* heap, int numberOfGCThreads) : GarbageCollector(heap) {
       
    this->numberOfGCThreads = numberOfGCThreads;
    
    pthread_mutex_init(&blockedMutex, nullptr);
    pthread_mutex_init(&markGlobalsMutex, nullptr);
    pthread_mutex_init(&markRootSetsMutex, nullptr);
    pthread_mutex_init(&markRootSetsCheckpointMutex, nullptr);
    pthread_cond_init(&doneMarkingRootSetsCondition, nullptr);
    pthread_cond_init(&blockedCondition, nullptr);
    //doneSignalling = false;
    //doneMarkingGlobals = false;
    blockedInterpreters = new vector<Interpreter*>();

    pthread_mutex_init(&nonEmptyWorklistsMutex, nullptr);
    pthread_cond_init(&waitingForWorkCondition, nullptr);
    nonEmptyWorklists = new vector<Worklist*>();
    //doneRequestCheckpoint = false;
    numberOfGCThreadsDoneMarking = 0; //Need to still check this
    
    //doneBlockingPages = false;
    pthread_mutex_init(&blockPagesMutex, nullptr);
    pthread_mutex_init(&pagesToRelocateMutex, nullptr);
    pthread_cond_init(&pagesToRelocateCondition, nullptr);
    pagesToRelocate = new vector<Page*>();
}

void PauselessCollector::Start() {
    for (int i=0; i < numberOfGCThreads; i++) {
        pthread_t tid = 0;
        pthread_create(&tid, NULL, &GCThread, NULL);
    }
}

void PauselessCollector::AddBlockedInterpreter(Interpreter* interpreter) {
    pthread_mutex_lock(&blockedMutex);
    blockedInterpreters->push_back(interpreter);
    pthread_cond_signal(&blockedCondition);
    pthread_mutex_unlock(&blockedMutex);
}

void PauselessCollector::SignalRootSetMarked() {
    pthread_mutex_lock(&markRootSetsCheckpointMutex);
    numberRootSetsMarked++;
    pthread_cond_signal(&doneMarkingRootSetsCondition);
    pthread_mutex_unlock(&markRootSetsCheckpointMutex);
}

// At the moment it is possible that a worklist which is already part of the list is added again
// This won't lead to inconsistencies but will lead to unwanted overhead -> need to think about better solution
// Something along the line of?
//         if (!worklist->empty())
//                AddNonEmptyWorklist(worklist);
//          worklist->addWork(newWork);
// Other option could be to use a set instead of a vector (how much overhead would such a thing bring along?)
// This method needs to be called by the NMT-trap
void PauselessCollector::AddNonEmptyWorklist(Worklist* worklist) {
    pthread_mutex_lock(&nonEmptyWorklistsMutex);
    nonEmptyWorklists->push_back(worklist);
    doneRequestCheckpoint = false;
    pthread_cond_signal(&waitingForWorkCondition);
    pthread_mutex_unlock(&nonEmptyWorklistsMutex);
}

void PauselessCollector::SignalSafepointReached() {
    numberOfMutatorsPassedSafepoint++;
    pthread_cond_signal(&waitingForWorkCondition);
}

/* TO DO: per cycle the following variables need to be reset:
        doneMarkingGlobals
        doneSignalling
        for each interpreter -> alreadyMarked 
        doneRequestCheckpoint */
void* PauselessCollector::GCThread(void*) {
    
    Worklist* localWorklist = new Worklist();
    Page* toPage;
    
    //------------------------
    // ROOT-SET MARKING
    //------------------------
    //doneMarkingGlobals = false;
    //doneSignalling = false;
    
    // one thread signals to all mutator threads to mark their root-sets
    // interpreters which are blocked are added to a vector containing pointers to these interpreters so that there root-set can be processed by a gc thread
    if (!doneSignalling && pthread_mutex_trylock(&markRootSetsMutex) == 0) {
        unique_ptr<vector<Interpreter*>> interpreters = GetUniverse()->GetInterpretersCopy();
        numberRootSetsToBeMarked = interpreters->size();
        for (vector<Interpreter*>::iterator it = interpreters->begin() ; it != interpreters->end(); ++it) {
            (*it)->TriggerMarkRootSet();
        }
        pthread_mutex_lock(&blockedMutex);
        doneSignalling = true;
        pthread_mutex_unlock(&blockedMutex);
        pthread_cond_broadcast(&blockedCondition);
        pthread_mutex_unlock(&markRootSetsMutex);
    }
    
    // one gc thread marks the globals
    if (!doneMarkingGlobals && pthread_mutex_trylock(&markGlobalsMutex) == 0) {
        GetUniverse()->MarkGlobals();
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
            blockedInterpreter->MarkRootSet();
        } else {
            pthread_mutex_unlock(&blockedMutex);
            break;
        }
    }
    
    // barrier that makes sure that all root-sets have been marked before continuing
    pthread_mutex_lock(&markRootSetsCheckpointMutex);
    while (numberRootSetsMarked != numberRootSetsToBeMarked) {
        pthread_cond_wait(&doneMarkingRootSetsCondition, &markRootSetsCheckpointMutex);
    }
    pthread_cond_broadcast(&doneMarkingRootSetsCondition);
    pthread_mutex_unlock(&markRootSetsCheckpointMutex);
    
    //------------------------
    // CONTINUE MARKING PHASE
    //------------------------
    while (true) {
       
        pthread_mutex_lock(&nonEmptyWorklistsMutex);
        numberOfGCThreadsDoneMarking++;
        while (nonEmptyWorklists->empty() && numberOfGCThreadsDoneMarking != numberOfGCThreads) {
            pthread_cond_wait(&waitingForWorkCondition, &nonEmptyWorklistsMutex);
        }
        if (!nonEmptyWorklists->empty()) {
            numberOfGCThreadsDoneMarking--;
            nonEmptyWorklists->back()->MoveWork(localWorklist);
            nonEmptyWorklists->pop_back();
            pthread_mutex_unlock(&nonEmptyWorklistsMutex);
        } else {
            // One gc-thread signals all mutators that a checkpoint is requested, no need to protect this by a mutex since only one thread can be active in this region at a time
            if (!doneRequestCheckpoint) {
                doneRequestCheckpoint = true;
                numberOfMutatorsPassedSafepoint = 0;
                unique_ptr<vector<Interpreter*>> interpreters = GetUniverse()->GetInterpretersCopy();
                numberOfMutators = interpreters->size();
                for (vector<Interpreter*>::iterator it = interpreters->begin() ; it != interpreters->end(); ++it) {
                    (*it)->RequestSafePoint();
                }
            }
            // Wait for safepoint passes
            while (numberOfMutators != numberOfMutatorsPassedSafepoint && nonEmptyWorklists->empty()) {
                pthread_cond_wait(&waitingForWorkCondition, &nonEmptyWorklistsMutex);
            }
            if (!nonEmptyWorklists->empty()) {
                numberOfGCThreadsDoneMarking--;
                nonEmptyWorklists->back()->MoveWork(localWorklist);
                nonEmptyWorklists->pop_back();
                pthread_mutex_unlock(&nonEmptyWorklistsMutex);
            } else {
                pthread_cond_signal(&waitingForWorkCondition); //perhaps I should do a broadcast?
                pthread_mutex_unlock(&nonEmptyWorklistsMutex);
                break;
            }
        }
        
        while (!localWorklist->Empty()) {
            AbstractVMObject* obj = localWorklist->GetWork();
            MarkObject(obj, localWorklist);
        }
        
    }
    
    //------------------------
    // RELOCATE PHASE
    //------------------------
    //doneBlockingPages = false;
    
    // Ensure that all mutators have switched the GC-trap off
    // After this we can identify the pages that should be relocated and mark them blocked
    // what about pages that are newly created at this point in time? They need to be aware of the fact that the GC-trap needs to be swiched off
    /*
    if (!doneBlockingPages && pthread_mutex_trylock(&blockPagesMutex) == 0) {
        unique_ptr<vector<Interpreter*>> interpreters = GetUniverse()->GetInterpretersCopy();
        for (vector<Interpreter*>::iterator it = interpreters->begin() ; it != interpreters->end(); ++it) {
            // no need for a barrier that waits till all mutators have reached safepoint as changing the boolean, at this point where we are guaranteed that no GC-trap will trigger anymore, has no consequence
            (*it)->DisableGCTrap();
        }
        for (std::vector<Page*>::iterator page = _HEAP->allPages->begin(); page != _HEAP->allPages->end(); ++page) {
            if (!(*page)->Used() && (((*page)->GetAmountOfLiveData() / _HEAP->pageSize) < 0.5)) {
                (*page)->Block();
                pthread_mutex_lock(&pagesToRelocateMutex);
                pagesToRelocate->push_back(*page);
                pthread_cond_signal(&pagesToRelocateCondition);
                pthread_mutex_unlock(&pagesToRelocateMutex);
            } else {
                (*page)->UnBlock();
            }
            
        }
        for (vector<Interpreter*>::iterator it = interpreters->begin() ; it != interpreters->end(); ++it) {
            (*it)->SignalEnableGCTrap();
        }
        doneBlockingPages = true;
        pthread_cond_broadcast(&pagesToRelocateCondition);
        pthread_mutex_unlock(&blockPagesMutex);
    }
    
    toPage = _HEAP->RequestPage();
    while (true) {
        pthread_mutex_lock(&pagesToRelocateMutex);
        while (!doneBlockingPages && pagesToRelocate->empty()) {
            pthread_cond_wait(&pagesToRelocateCondition, &pagesToRelocateMutex);
        }
        if (!pagesToRelocate->empty()) {
            Page* fromPage = pagesToRelocate->back();
            pagesToRelocate->pop_back();
            pthread_mutex_unlock(&pagesToRelocateMutex);
            fromPage->RelocatePage(toPage);
            //RelocatePage(fromPage, toPage);
        } else {
            pthread_mutex_unlock(&pagesToRelocateMutex);
            break;
        }
    } */
    
    return nullptr;
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
