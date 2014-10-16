#include "StopTheWorldHeap.h"

#include "GenerationalCollector.h"
#include "../../vmobjects/AbstractObject.h"
#include "../../vm/Universe.h"

#include <string.h>
#include <iostream>

#if GC_TYPE != PAUSELESS

StopTheWorldHeap::StopTheWorldHeap(long objectSpaceSize, long pageSize) : PagedHeap(objectSpaceSize, pageSize) {
    this->gcTriggered = false;
    this->threadCount = 0;
    this->readyForGCThreads = 0;
    pthread_mutex_init(&doCollect, NULL);
    pthread_mutex_init(&threadCountMutex, NULL);
    pthread_cond_init(&stopTheWorldCondition, NULL);
    pthread_cond_init(&mayProceed, NULL);

}

void StopTheWorldHeap::checkCollectionTreshold() {
    if (availablePages->size() < 4) { //this is for the moment a bit randomly chosen
        triggerGC();
    }
    //if (nextFreePagePosition > collectionLimit)
    //triggerGC();
}

void StopTheWorldHeap::IncrementThreadCount() {
    pthread_mutex_lock(&threadCountMutex);
    threadCount++;
    pthread_mutex_unlock(&threadCountMutex);
}

void StopTheWorldHeap::DecrementThreadCount() {
    pthread_mutex_lock(&threadCountMutex);
    threadCount--;
    pthread_mutex_unlock(&threadCountMutex);
}

// actual function of these is to signal that a thread is blocked
void StopTheWorldHeap::IncrementWaitingForGCThreads() {
    pthread_mutex_lock(&threadCountMutex);
    readyForGCThreads++;
    pthread_cond_signal(&stopTheWorldCondition);
    pthread_mutex_unlock(&threadCountMutex);
}

void StopTheWorldHeap::DecrementWaitingForGCThreads() {
    pthread_mutex_lock(&threadCountMutex);
    readyForGCThreads--;
    pthread_cond_signal(&stopTheWorldCondition);
    pthread_mutex_unlock(&threadCountMutex);
}

void StopTheWorldHeap::FullGC() {
    // one thread is going to do the GC
    if (pthread_mutex_trylock(&doCollect) == 0) {
        // all threads must have reached a safe point for the GC to take place
        pthread_mutex_lock(&threadCountMutex);
        while (threadCount != readyForGCThreads) {
            pthread_cond_wait(&stopTheWorldCondition, &threadCountMutex);
        }
        pthread_mutex_unlock(&threadCountMutex);
        // all threads have reached a safe point
        static_cast<StopTheWorldCollector*>(gc)->Collect();
        // signal all the threads that the GC is completed
        pthread_cond_broadcast(&mayProceed);
        pthread_mutex_unlock(&doCollect);
        // other threads signal the barrier that for them the GC may take place
    } else {
        pthread_mutex_lock(&threadCountMutex);
        readyForGCThreads++;
        pthread_cond_signal(&stopTheWorldCondition);
        while (gcTriggered) {
            pthread_cond_wait(&mayProceed, &threadCountMutex);
        }
        readyForGCThreads--;
        pthread_mutex_unlock(&threadCountMutex);
    }
}

#endif
