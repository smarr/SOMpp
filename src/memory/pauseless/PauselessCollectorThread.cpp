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

#include <misc/debug.h>

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

vector<Worklist*>* PauselessCollectorThread::nonEmptyWorklists = new vector<Worklist*>();
pthread_mutex_t PauselessCollectorThread::nonEmptyWorklistsMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  PauselessCollectorThread::waitingForWorkCondition = PTHREAD_COND_INITIALIZER;
pthread_cond_t  PauselessCollectorThread::waitingForCheckpointCondition = PTHREAD_COND_INITIALIZER;
int PauselessCollectorThread::numberOfGCThreadsDoneMarking = 0;
int PauselessCollectorThread::numberOfMutatorsPassedSafepoint;
int PauselessCollectorThread::numberOfMutators = 0;
bool PauselessCollectorThread::checkpointRequested = false;

bool PauselessCollectorThread::doneBlockingPages = false;
pthread_mutex_t PauselessCollectorThread::blockPagesMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t PauselessCollectorThread::pagesToRelocateMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t PauselessCollectorThread::pagesToRelocateCondition = PTHREAD_COND_INITIALIZER;
vector<Page*>* PauselessCollectorThread::pagesToRelocate = new vector<Page*>();
vector<Page*>* PauselessCollectorThread::pagesToUnblock = new vector<Page*>();

// FOR DEBUGGING PURPOSES
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
    if (obj->GetGCField())
        return;
    Page* page = _HEAP->allPages->at(((size_t)obj - (size_t)_HEAP->memoryStart) / _HEAP->pageSize);
    
    assert(REFERENCE_NMT_VALUE(obj) == false);
    obj->GetObjectSize();
    
    page->AddAmountLiveData(obj->GetObjectSize());
    obj->SetGCField(MASK_OBJECT_IS_MARKED);
    obj->MarkReferences();
}

void PauselessCollectorThread::CheckMarkingOfObject(VMOBJECT_PTR obj) {
    size_t pageNumber = ((size_t)Untag(obj) - (size_t)(_HEAP->GetMemoryStart())) / _HEAP->GetPageSize();
    Page* page = _HEAP->allPages->at(pageNumber);
    bool objInFullPage;
    if (std::find(_HEAP->fullPages->begin(), _HEAP->fullPages->end(), page) != _HEAP->fullPages->end())
        objInFullPage = true;
    else
        objInFullPage = false;
    //assert(obj->GetGCField() || !objInFullPage);
    
    if (obj->GetGCField2())
        return;
    
    obj->SetGCField(MASK_OBJECT_IS_MARKED);
    obj->SetGCField2(MASK_OBJECT_IS_MARKED);
    obj->CheckMarking(CheckMarkingOfObject);
}


void PauselessCollectorThread::CheckMarking() {
    _UNIVERSE->CheckMarkingGlobals(CheckMarkingOfObject);
    unique_ptr<vector<Interpreter*>> interpreters = _UNIVERSE->GetInterpretersCopy();
    for (vector<Interpreter*>::iterator it = interpreters->begin() ; it != interpreters->end(); ++it) {
        (*it)->CheckMarking(CheckMarkingOfObject);
    }
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
    if (checkpointRequested) {
        checkpointRequested = false;
        pthread_cond_signal(&waitingForCheckpointCondition);
    } else
        pthread_cond_signal(&waitingForWorkCondition);
    pthread_mutex_unlock(&nonEmptyWorklistsMutex);
}

void PauselessCollectorThread::SignalSafepointReached() {
    pthread_mutex_lock(&nonEmptyWorklistsMutex);
    numberOfMutatorsPassedSafepoint++;
    pthread_cond_signal(&waitingForCheckpointCondition);
    pthread_mutex_unlock(&nonEmptyWorklistsMutex);
}

void PauselessCollectorThread::Collect() {
    
    while (true) {
        
        expectedNMT = !expectedNMT;
        PagedHeap* test = _HEAP;
        
        // DEBUGGING PURPOSES: STOP THE WORLD
        _HEAP->TriggerPause();
        pthread_mutex_lock(&(_HEAP->threadCountMutex));
        while (_UNIVERSE->GetInterpretersCopy()->size() != _HEAP->readyForPauseThreads) {
            pthread_cond_wait(&(_HEAP->stopTheWorldCondition), &(_HEAP->threadCountMutex));
        }
        pthread_mutex_unlock(&(_HEAP->threadCountMutex));
        CheckMarking();
        _HEAP->ResetPause();
        pthread_cond_broadcast(&(_HEAP->mayProceed));
        
        sync_out(ostringstream() << "[GC] Start RootSet Marking");
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
        // this says nothing about the marking of the globals, no guarantee that this is finished
        pthread_mutex_lock(&markRootSetsCheckpointMutex);
        while (numberRootSetsMarked != numberRootSetsToBeMarked && !doneMarkingGlobals) {
            pthread_cond_wait(&doneMarkingRootSetsCondition, &markRootSetsCheckpointMutex);
        }
        pthread_cond_broadcast(&doneMarkingRootSetsCondition);
        pthread_mutex_unlock(&markRootSetsCheckpointMutex);
        
        
        sync_out(ostringstream() << "[GC] Start Marking Phase");
        
        //------------------------
        // CONTINUE MARKING PHASE
        //------------------------
        while (true) {
            
            while (!worklist.Empty()) {
                VMOBJECT_PTR obj = worklist.GetWork();
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
                // all threads have run out of work -> request a safepoint
                unique_ptr<vector<Interpreter*>> interpreters = _UNIVERSE->GetInterpretersCopy();
                for (vector<Interpreter*>::iterator it = interpreters->begin() ; it != interpreters->end(); ++it) {
                    (*it)->RequestSafePoint();
                }
                numberOfMutatorsPassedSafepoint = 0;
                numberOfMutators = interpreters->size();
                checkpointRequested = true;
                // after this, this thread should also wait to see whether or not all threads succesfully pass the safepoint
                while (numberOfMutators != numberOfMutatorsPassedSafepoint && nonEmptyWorklists->empty()) {
                    pthread_cond_wait(&waitingForCheckpointCondition, &nonEmptyWorklistsMutex);
                }
                if (numberOfMutators == numberOfMutatorsPassedSafepoint) {
                    //everything is done and all threads should
                    //pthread_cond_signal() -> should still be done but testing happens with one gc thread for the moment
                    
                    //assert(nonEmptyWorklists->empty());
                    
                    pthread_mutex_unlock(&nonEmptyWorklistsMutex);
                    checkpointRequested = false; //setup for next cycle
                    break;
                } else {
                    nonEmptyWorklists->back()->MoveWork(&worklist);
                    nonEmptyWorklists->pop_back();
                    numberOfGCThreadsDoneMarking--;
                    pthread_mutex_unlock(&nonEmptyWorklistsMutex);
                }
            }
        }
        
        sync_out(ostringstream() << "[GC] Start Relocate Phase");
        
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
                _HEAP->AddEmptyPage(*page);
            }
            pagesToUnblock->clear();
            // block pages that will be subject for relocation
            for (vector<Page*>::iterator page = _HEAP->fullPages->begin(); page != _HEAP->fullPages->end(); ++page) {
                (*page)->Block();
                pagesToRelocate->push_back(*page);
            }
            _HEAP->fullPages->clear();  //still needs a lock
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
        } else {
            pthread_mutex_lock(&pagesToRelocateMutex);
            while (!doneBlockingPages)
                pthread_cond_wait(&pagesToRelocateCondition, &pagesToRelocateMutex);
            pthread_mutex_unlock(&pagesToRelocateMutex);
        }
        
        // do I really need this?
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

        

        
        //reset some values for next cycle:
        doneSignalling = false;
        doneBlockingPages = false;
        doneMarkingGlobals = false;
        numberOfGCThreadsDoneMarking = 0;
        
        sync_out(ostringstream() << "[GC] End of cycle");
        sync_out(ostringstream() << "=================");

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




/*
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
*/
#endif
