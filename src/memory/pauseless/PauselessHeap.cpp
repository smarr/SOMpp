#include "PauselessHeap.h"
#include "../Page.h"
//#include "PauselessCollector.h"
#include "../../vmobjects/AbstractObject.h"
#include "../../vm/Universe.h"
#include "PauselessCollectorThread.h"

#include <string.h>
#include <iostream>
#include <pthread.h>

#include <sys/mman.h>

#if GC_TYPE == PAUSELESS

#define NUMBER_OF_GC_THREADS 1

PauselessHeap::PauselessHeap(long objectSpaceSize, long pageSize) : PagedHeap(objectSpaceSize, pageSize) {
    pthread_key_create(&pauselessCollectorThread, NULL);
    pthread_mutex_init(&gcTrapEnabledMutex, NULL);
    pthread_cond_init(&gcTrapEnabledCondition, NULL);
    PauselessCollectorThread::SetNumberOfGCThreads(NUMBER_OF_GC_THREADS);
    
    // FOR DEBUGGING PURPOSES
    this->pauseTriggered = false;
    this->readyForPauseThreads = 0;
    pthread_mutex_init(&doCollect, NULL);
    pthread_mutex_init(&threadCountMutex, NULL);
    pthread_cond_init(&stopTheWorldCondition, NULL);
    pthread_cond_init(&mayProceed, NULL);
}

void PauselessHeap::Start() {
    for (int i=0; i < NUMBER_OF_GC_THREADS; i++) {
        pthread_t tid = 0;
        pthread_create(&tid, NULL, &ThreadForGC, NULL);
    }
}

void* PauselessHeap::ThreadForGC(void*) {
    PauselessCollectorThread* gcThread = new PauselessCollectorThread();
    _HEAP->AddGCThread(gcThread);
    gcThread->Collect();
    return nullptr;
}

PauselessCollectorThread* PauselessHeap::GetGCThread() {
    return (PauselessCollectorThread*)pthread_getspecific(pauselessCollectorThread);
}

void PauselessHeap::AddGCThread(PauselessCollectorThread* gcThread) {
    pthread_setspecific(pauselessCollectorThread, gcThread);
}

void PauselessHeap::SignalRootSetMarked() {
    PauselessCollectorThread::SignalRootSetMarked();
}

void PauselessHeap::SignalInterpreterBlocked(Interpreter* interpreter) {
    PauselessCollectorThread::AddBlockedInterpreter(interpreter);
}

void PauselessHeap::SignalSafepointReached(bool* safePointRequested) {
    PauselessCollectorThread::SignalSafepointReached(safePointRequested);
}

void PauselessHeap::SignalGCTrapEnabled() {
    pthread_mutex_lock(&gcTrapEnabledMutex);
    numberOfMutatorsWithEnabledGCTrap++;
    pthread_cond_signal(&gcTrapEnabledCondition);
    pthread_mutex_unlock(&gcTrapEnabledMutex);
}

// FOR DEBUGGING PURPOSES
void PauselessHeap::Pause() {
    pthread_mutex_lock(&threadCountMutex);
    readyForPauseThreads++;
    pthread_cond_signal(&stopTheWorldCondition);
    while (pauseTriggered) {
        pthread_cond_wait(&mayProceed, &threadCountMutex);
    }
    readyForPauseThreads--;
    pthread_mutex_unlock(&threadCountMutex);
}

bool PauselessHeap::IsPauseTriggered(void) {
    return pauseTriggered;
}

void PauselessHeap::TriggerPause() {
    pauseTriggered = true;
}

void PauselessHeap::ResetPause() {
    pauseTriggered = false;
}

#endif