#include "PauselessCollectorThread.h"

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


#define REFERENCE_NMT_VALUE(REFERENCE) (((reinterpret_cast<intptr_t>(REFERENCE) & MASK_OBJECT_NMT) == 0) ? false : true)

int PauselessCollectorThread::numberOfGCThreads;

pthread_mutex_t PauselessCollectorThread::blockedMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t PauselessCollectorThread::markGlobalsMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t PauselessCollectorThread::markRootSetsMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t PauselessCollectorThread::markRootSetsCheckpointMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  PauselessCollectorThread::doneMarkingRootSetsCondition = PTHREAD_COND_INITIALIZER;
pthread_cond_t  PauselessCollectorThread::blockedCondition = PTHREAD_COND_INITIALIZER;
int PauselessCollectorThread::numberRootSetsMarked = 0;
int PauselessCollectorThread::numberRootSetsToBeMarked = 0;
bool PauselessCollectorThread::doneSignalling = false;
bool PauselessCollectorThread::doneMarkingGlobals = false;
vector<Interpreter*>* PauselessCollectorThread::blockedInterpreters = new vector<Interpreter*>();

pthread_mutex_t PauselessCollectorThread::nonEmptyWorklistsMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  PauselessCollectorThread::waitingForWorkCondition = PTHREAD_COND_INITIALIZER;
vector<Worklist*>* PauselessCollectorThread::nonEmptyWorklists = new vector<Worklist*>();
bool PauselessCollectorThread::doneRequestCheckpoint = false;
int PauselessCollectorThread::numberOfGCThreadsDoneMarking = 0;
atomic<int> PauselessCollectorThread::numberOfMutatorsPassedSafepoint;
int PauselessCollectorThread::numberOfMutators = 0;

bool PauselessCollectorThread::doneBlockingPages = false;
pthread_mutex_t PauselessCollectorThread::blockPagesMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t PauselessCollectorThread::pagesToRelocateMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t PauselessCollectorThread::pagesToRelocateCondition = PTHREAD_COND_INITIALIZER;
vector<Page*>* PauselessCollectorThread::pagesToRelocate = new vector<Page*>();
vector<Page*>* PauselessCollectorThread::pagesToUnblock = new vector<Page*>();

// TEST VARIABLES
pthread_mutex_t PauselessCollectorThread::endMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t PauselessCollectorThread::endCondition = PTHREAD_COND_INITIALIZER;
int PauselessCollectorThread::numberReachingEnd = 0;
bool PauselessCollectorThread::done = false;
int PauselessCollectorThread::numberOfCycles = 0;

void PauselessCollectorThread::SetNumberOfGCThreads(int numberOfThreads) {
    numberOfGCThreads = numberOfThreads;
}

PauselessCollectorThread::PauselessCollectorThread() : BaseThread() {}

void PauselessCollectorThread::AddGCWork(AbstractVMObject* work) {
    worklist.AddWorkGC(work);
}

void PauselessCollectorThread::MarkObject(VMOBJECT_PTR obj) {
    assert(Universe::IsValidObject(obj));
    if (obj->GetGCField() & MASK_OBJECT_IS_MARKED)
        return;
    Page* page = _HEAP->allPages->at(((size_t)obj - (size_t)_HEAP->memoryStart) / _HEAP->pageSize);
    
    assert(REFERENCE_NMT_VALUE(obj) == false);
    obj->GetObjectSize();
    
    page->AddAmountLiveData(obj->GetObjectSize());
    obj->SetGCField(MASK_OBJECT_IS_MARKED);
    obj->MarkReferences();
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
    nonEmptyWorklists->push_back(worklist);
    doneRequestCheckpoint = false;
    pthread_cond_signal(&waitingForWorkCondition);
    pthread_mutex_unlock(&nonEmptyWorklistsMutex);
}

void PauselessCollectorThread::SignalSafepointReached() {
    numberOfMutatorsPassedSafepoint++;
    pthread_cond_signal(&waitingForWorkCondition);
}

void PauselessCollectorThread::Collect() {
    
    while (true) {
        
        //------------------------
        // ROOT-SET MARKING
        //------------------------
        if (!doneSignalling && pthread_mutex_trylock(&markRootSetsMutex) == 0) {
            numberOfCycles++;
            
            unique_ptr<vector<Interpreter*>> interpreters = _UNIVERSE->GetInterpretersCopy();
            numberRootSetsToBeMarked = interpreters->size();
            numberRootSetsMarked = 0;
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
            _UNIVERSE->MarkGlobals();
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
        while (numberRootSetsMarked != numberRootSetsToBeMarked) {
            pthread_cond_wait(&doneMarkingRootSetsCondition, &markRootSetsCheckpointMutex);
        }
        pthread_cond_broadcast(&doneMarkingRootSetsCondition);
        pthread_mutex_unlock(&markRootSetsCheckpointMutex);
        
        
        //------------------------
        // CONTINUE MARKING PHASE
        //------------------------
        while (true) {
        
            while (!worklist.Empty()) {
                VMOBJECT_PTR obj = worklist.GetWork();
                MarkObject(obj);
            }
            
            pthread_mutex_lock(&nonEmptyWorklistsMutex);
            numberOfGCThreadsDoneMarking++;
            while (nonEmptyWorklists->empty() && numberOfGCThreadsDoneMarking != numberOfGCThreads) {
                pthread_cond_wait(&waitingForWorkCondition, &nonEmptyWorklistsMutex);
            }
            if (!nonEmptyWorklists->empty()) {
                numberOfGCThreadsDoneMarking--;
                nonEmptyWorklists->back()->MoveWork(&worklist);
                nonEmptyWorklists->pop_back();
                pthread_mutex_unlock(&nonEmptyWorklistsMutex);
            } else {
                // One gc-thread signals all mutators that a checkpoint is requested, no need to protect this by a mutex since only one thread can be active in this region at a time
                if (!doneRequestCheckpoint) {
                    doneRequestCheckpoint = true;
                    numberOfMutatorsPassedSafepoint = 0;
                    unique_ptr<vector<Interpreter*>> interpreters = _UNIVERSE->GetInterpretersCopy();
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
                    nonEmptyWorklists->back()->MoveWork(&worklist);
                    nonEmptyWorklists->pop_back();
                    pthread_mutex_unlock(&nonEmptyWorklistsMutex);
                } else {
                    pthread_cond_signal(&waitingForWorkCondition); //perhaps I should do a broadcast?
                    pthread_mutex_unlock(&nonEmptyWorklistsMutex);
                    break;
                }
            }
        }

        //------------------------
        // RELOCATE PHASE
        //------------------------
        if (!doneBlockingPages && pthread_mutex_trylock(&blockPagesMutex) == 0) {
            // disable GC-trap from running interpreters
            // _UNIVERSE->GC_TRAP_STATUS = FALSE;
            unique_ptr<vector<Interpreter*>> interpreters = _UNIVERSE->GetInterpretersCopy();
            for (vector<Interpreter*>::iterator it = interpreters->begin() ; it != interpreters->end(); ++it) {
                (*it)->DisableGCTrap();
            }
            _HEAP->numberOfMutatorsWithEnabledGCTrap = 0;
            // unblock pages that were blocked during the previous cycle
            for (vector<Page*>::iterator page = pagesToUnblock->begin(); page != pagesToUnblock->end(); ++page) {
                (*page)->UnBlock();
            }
            pagesToUnblock->clear();
            // determine which pages should be blocked during this cycle -> should be changed
            for (std::vector<Page*>::iterator page = _HEAP->fullPages->begin(); page != _HEAP->fullPages->end(); ++page) {
                if ((*page)->GetAmountOfLiveData() / _HEAP->pageSize < 0.5) { //value should not be hardcoded
                    (*page)->Block();
                    pthread_mutex_lock(&pagesToRelocateMutex);
                    pagesToRelocate->push_back(*page);
                    pthread_cond_signal(&pagesToRelocateCondition);
                    pthread_mutex_unlock(&pagesToRelocateMutex);
                    pagesToUnblock->push_back(*page);
                }
            }
            // enable the GC-trap again
            // _UNIVERSE->GC_TRAP_STATUS = TRUE;
            interpreters = _UNIVERSE->GetInterpretersCopy();
            _HEAP->numberOfMutatorsNeedEnableGCTrap = interpreters->size();
            for (vector<Interpreter*>::iterator it = interpreters->begin() ; it != interpreters->end(); ++it) {
                (*it)->SignalEnableGCTrap();
            }
            doneBlockingPages = true;
            pthread_cond_broadcast(&pagesToRelocateCondition);
            pthread_mutex_unlock(&blockPagesMutex);
        }
        
        while (true) {
            pthread_mutex_lock(&pagesToRelocateMutex);
            while (!doneBlockingPages && pagesToRelocate->empty()) {
                pthread_cond_wait(&pagesToRelocateCondition, &pagesToRelocateMutex);
            }
            if (!pagesToRelocate->empty()) {
                Page* fromPage = pagesToRelocate->back();
                pagesToRelocate->pop_back();
                pthread_mutex_unlock(&pagesToRelocateMutex);
                fromPage->RelocatePage();
                _HEAP->AddEmptyPage(fromPage);
            } else {
                pthread_mutex_unlock(&pagesToRelocateMutex);
                break;
            }
        }
        
        /*
        pthread_mutex_lock(&endMutex);
        numberReachingEnd++;
        while (numberOfGCThreads != numberReachingEnd) {
            pthread_cond_wait(&endCondition, &endMutex);
        }
        pthread_cond_broadcast(&endCondition);
        pthread_mutex_unlock(&endMutex); */
        
        //reset some values for next cycle:
        doneSignalling = false;
        doneRequestCheckpoint = false;
        doneBlockingPages = false;
        numberOfGCThreadsDoneMarking = 0;
        numberReachingEnd--;

    } //end of cycle

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
