#include "PauselessHeap.h"
#include "../Page.h"
#include "PauselessCollector.h"
#include "../../vmobjects/AbstractObject.h"
#include "../../vm/Universe.h"

#include <string.h>
#include <iostream>

#include <sys/mman.h>

#if GC_TYPE == PAUSELESS

#define NUMBER_OF_GC_THREADS 4

PauselessHeap::PauselessHeap(long objectSpaceSize, long pageSize) {
    gc = new PauselessCollector(this, NUMBER_OF_GC_THREADS);
}

void PauselessHeap::SignalRootSetMarked() {
    static_cast<PauselessCollector*>(gc)->SignalRootSetMarked();
}

void PauselessHeap::SignalInterpreterBlocked(Interpreter* interpreter) {
    static_cast<PauselessCollector*>(gc)->AddBlockedInterpreter(interpreter);
}

void PauselessHeap::SignalSafepointReached() {
    static_cast<PauselessCollector*>(gc)->SignalSafepointReached();
}

#endif



/*
 Page* PauselessHeap::GCRequestPage() {
 Page* newPage;
 pthread_mutex_lock(&pageRequestMutex);
 if (availablePages->empty()) {
 newPage = new Page(nextFreePagePosition, this);
 assert(newPage);
 allPages->push_back(newPage);
 nextFreePagePosition = (void*) ((size_t)nextFreePagePosition + pageSize);
 checkCollectionTreshold();
 //if (nextFreePagePosition > collectionLimit) triggerGC();
 } else {
 newPage = availablePages->back();
 availablePages->pop_back();
 }
 pthread_mutex_unlock(&pageRequestMutex);
 return newPage;
 }
 
 */
