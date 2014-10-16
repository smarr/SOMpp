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

#define NUMBER_OF_GC_THREADS 4

PauselessHeap::PauselessHeap(long objectSpaceSize, long pageSize) : PagedHeap(objectSpaceSize, pageSize) {
    //gc = new PauselessCollector(this, NUMBER_OF_GC_THREADS); //perhaps I should change this?
    pthread_key_create(&pauselessCollectorThread, NULL);
}

void PauselessHeap::SignalRootSetMarked() {
    PauselessCollectorThread::SignalRootSetMarked();
    //static_cast<PauselessCollector*>(gc)->SignalRootSetMarked();
}

void PauselessHeap::SignalInterpreterBlocked(Interpreter* interpreter) {
    PauselessCollectorThread::AddBlockedInterpreter(interpreter);
    //static_cast<PauselessCollector*>(gc)->AddBlockedInterpreter(interpreter);
}

void PauselessHeap::SignalSafepointReached() {
    PauselessCollectorThread::SignalSafepointReached();
    //static_cast<PauselessCollector*>(gc)->SignalSafepointReached();
}

void PauselessHeap::SignalGCTrapEnabled() {
    pthread_cond_signal(&gcTrapEnabledCondition);
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

#endif