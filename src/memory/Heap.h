#pragma once
#ifndef HEAP_H_
#define HEAP_H_

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
using namespace std;
//macro to access the heap
#define _HEAP Heap::GetHeap()

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

class Heap {
    friend class GarbageCollector;

public:
    static inline HEAP_CLS* GetHeap();
    static void InitializeHeap(long objectSpaceSize = 1048576);
    static void DestroyHeap();
    Heap(long objectSpaceSize = 1048576);
    ~Heap();
    inline void triggerGC(void);
    inline void resetGCTrigger(void);
    bool isCollectionTriggered(void);
    void FullGC();
    inline void FreeObject(AbstractVMObject* o);
    void IncrementThreadCount();
    void DecrementThreadCount();
    void IncrementWaitingForGCThreads();
    void DecrementWaitingForGCThreads();
    
protected:
    GarbageCollector* gc;
    pthread_mutex_t doCollect;
    pthread_mutex_t threadCountMutex;
    pthread_mutex_t allocationLock;
    pthread_cond_t stopTheWorldCondition;
    pthread_cond_t mayProceed;
private:
    static HEAP_CLS* theHeap;
    //flag that shows if a Collection is triggered
    volatile bool gcTriggered;
    volatile int threadCount = 0;
    volatile int readyForGCThreads = 0;
};

HEAP_CLS* Heap::GetHeap() {
    return theHeap;
}

void Heap::triggerGC(void) {
    gcTriggered = true;
}

inline bool Heap::isCollectionTriggered(void) {
    return gcTriggered;
}

void Heap::resetGCTrigger(void) {
    gcTriggered = false;
}

void Heap::FreeObject(AbstractVMObject* obj) {
    free(obj);
}
#endif
