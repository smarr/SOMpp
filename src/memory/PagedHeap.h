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

# ifndef GC_TYPE
# error GC_TYPE is not defined
# endif

#if GC_TYPE==GENERATIONAL
class GenerationalHeap;
#define HEAP_CLS GenerationalHeap
#elif GC_TYPE==COPYING
class CopyingHeap;
#define HEAP_CLS CopyingHeap
#elif GC_TYPE==MARK_SWEEP
class MarkSweepHeap;
#define HEAP_CLS MarkSweepHeap
#elif GC_TYPE==PAUSELESS
class PauselessHeap;
#define HEAP_CLS PauselessHeap
#else
  # define VALUE_TO_STRING(x) #x
  # define VALUE(x) VALUE_TO_STRING(x)
  # define VAR_NAME_VALUE(var) #var "=" VALUE(var)

  # pragma message(VAR_NAME_VALUE(GC_TYPE))
  # error The given value of GC_TYPE is not known
#endif

class PagedHeap {
    friend class GarbageCollector;
    friend class PauselessCollector; //this should probably also be made compile time dependend
    friend class PauselessCollectorThread;
    friend class Page;
    
public:
    static FORCE_INLINE HEAP_CLS* GetHeap() { return theHeap; }
    static void InitializeHeap(long, long);
    static void DestroyHeap();
    PagedHeap(long, long);
    ~PagedHeap();
    size_t GetMaxObjectSize();

    inline void FreeObject(AbstractVMObject* o);
    
    Page* RequestPage();
    void RelinquishPage(Page*);
    //void RelinquishFullPage(Page*);
    void AddEmptyPage(Page*);
    
    virtual void checkCollectionTreshold() {};
    
    vector<Page*>* allPages;
    
    int GetNumberOfCycles();
    
protected:
    GarbageCollector* gc;
    pthread_mutex_t fullPagesMutex;
    pthread_mutex_t availablePagesMutex;
    //void* nextFreePagePosition;
    //void* collectionLimit;
    void* memoryStart;
    size_t memoryEnd;
    //vector<Page*>* allPages;
    vector<Page*>* availablePages;
    vector<Page*>* fullPages;
    
private:
    void CreatePages();
    
    static HEAP_CLS* theHeap;
    size_t maxObjSize;
};

void PagedHeap::FreeObject(AbstractVMObject* obj) {
    free(obj);
}
#endif
