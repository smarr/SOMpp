#pragma once
#ifndef PAGEDHEAP_H_
#define PAGEDHEAP_H_

/*
 *
 *
 Copyright (c) 2007 Michael Haupt, Tobias Pape, Arne Bergmann
 Software Architecture Group, Hasso Plattner Institute, Potsdam, Germany
 http://www.hpi.uni-potsdam.de/swa/

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

#include <vector>
#include <pthread.h>
#include <set>
#include <cstdlib>
#include "GarbageCollector.h"
#include "../misc/defs.h"
#include "../vmobjects/ObjectFormats.h"

class AbstractVMObject;
class Page;
using namespace std;
//macro to access the heap
#define _HEAP PagedHeap::GetHeap()

#if GC_TYPE==GENERATIONAL
class GenerationalHeap;
#define HEAP_CLS GenerationalHeap
#elif GC_TYPE == COPYING
class CopyingHeap;
#define HEAP_CLS CopyingHeap
#elif GC_TYPE == MARK_SWEEP
class MarkSweepHeap;
#define HEAP_CLS MarkSweepHeap
#endif

class PagedHeap {
    friend class GarbageCollector;
    friend class Page;
    
public:
    static inline HEAP_CLS* GetHeap();
    static void InitializeHeap(long objectSpaceSize = 4194304,long pageSize = 32768);
    static void DestroyHeap();
    PagedHeap(long objectSpaceSize = 4194304, long pageSize = 32768);
    ~PagedHeap();
    size_t GetMaxObjectSize();
    inline void triggerGC(void);
    inline void resetGCTrigger(void);
    bool isCollectionTriggered(void);
    void FullGC();
    inline void FreeObject(AbstractVMObject* o);
    void IncrementThreadCount();
    void DecrementThreadCount();
    void IncrementWaitingForGCThreads();
    void DecrementWaitingForGCThreads();
    
    Page* RequestPage();
    void RelinquishPage(Page*);
    void RelinquishFullPage(Page*);
    
protected:
    long pageSize;
    GarbageCollector* gc;
    pthread_mutex_t doCollect;
    pthread_mutex_t threadCountMutex;
    pthread_cond_t stopTheWorldCondition;
    pthread_cond_t mayProceed;
    void* nextFreePagePosition;
    void* collectionLimit;
    void* memoryStart;
    size_t memoryEnd;
    vector<Page*>* allPages;
    vector<Page*>* availablePages;
    vector<Page*>* fullPages;
    
private:
    static HEAP_CLS* theHeap;
    volatile bool gcTriggered;
    volatile int threadCount = 0;
    volatile int readyForGCThreads = 0;
    size_t maxObjSize;
};

HEAP_CLS* PagedHeap::GetHeap() {
    return theHeap;
}

void PagedHeap::triggerGC(void) {
    gcTriggered = true;
}

inline bool PagedHeap::isCollectionTriggered(void) {
    return gcTriggered;
}

void PagedHeap::resetGCTrigger(void) {
    gcTriggered = false;
}

void PagedHeap::FreeObject(AbstractVMObject* obj) {
    free(obj);
}
#endif
