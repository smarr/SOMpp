#pragma once

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

#include <mutex>
#include <vector>
#include <pthread.h>
#include <set>
#include <cstdlib>
#include "GarbageCollector.h"

#include <misc/defs.h>
#include <memory/Heap.h>
#include <vmobjects/ObjectFormats.h>

class AbstractVMObject;

using namespace std;

class PauselessPage;
class PauselessHeap;

class PagedHeap : public Heap<PauselessHeap> {
    friend class GarbageCollector<HEAP_CLS>;
    friend class PauselessCollector; //this should probably also be made compile time dependend
    friend class PauselessCollectorThread;
    friend class PauselessPage;
    
public:
    PagedHeap(size_t pageSize, size_t maxHeapSize);
    ~PagedHeap();
    size_t GetMaxObjectSize();

    inline void FreeObject(AbstractVMObject* o);
    
    PauselessPage* RequestPage();
    void RelinquishPage(PauselessPage*);
    //void RelinquishFullPage(PauselessPage*);
    void AddEmptyPage(PauselessPage*);
    
    virtual void checkCollectionTreshold() {};
    
    vector<PauselessPage*>* allPages;
    
protected:
    pthread_mutex_t fullPagesMutex;

    mutex availablePages_mutex;
    
    //void* nextFreePagePosition;
    //void* collectionLimit;
    void* memoryStart;
    size_t memoryEnd;
    //vector<Page*>* allPages;
    vector<PauselessPage*>* availablePages;
    vector<PauselessPage*>* fullPages;
    
private:
    void CreatePages();
    
    static HEAP_CLS* theHeap;
    size_t maxObjSize;
};

void PagedHeap::FreeObject(AbstractVMObject* obj) {
    free(obj);
}
