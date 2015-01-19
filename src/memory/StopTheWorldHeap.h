#pragma once

#include <misc/defs.h>
#include <assert.h>
#if GC_TYPE!=PAUSELESS

#include "PagedHeap.h"

class StopTheWorldHeap : public PagedHeap {
    
public:
    
    StopTheWorldHeap(long, long);
    
    void FullGC();
    
    void IncrementThreadCount();
    void DecrementThreadCount();
    void IncrementWaitingForGCThreads();
    void DecrementWaitingForGCThreads();

    inline void triggerGC(void);
    inline void resetGCTrigger(void);
    bool isCollectionTriggered(void);
    void checkCollectionTreshold();
    
private:
    
    volatile bool gcTriggered;
    volatile int threadCount = 0;
    volatile int readyForGCThreads = 0;
    pthread_mutex_t doCollect;
    pthread_mutex_t threadCountMutex;
    pthread_cond_t stopTheWorldCondition;
    pthread_cond_t mayProceed;

};

void StopTheWorldHeap::triggerGC(void) {
    gcTriggered = true;
}

void StopTheWorldHeap::resetGCTrigger(void) {
    gcTriggered = false;
}

inline bool StopTheWorldHeap::isCollectionTriggered(void) {
    return gcTriggered;
}

template<typename T>
inline typename T::Loaded* ReadBarrier(T* reference) {
    return (typename T::Loaded*) reference;
}

template<typename T>
inline typename T::Stored* WriteBarrier(T* reference) {
    return (typename T::Stored*) reference;
}

#endif
