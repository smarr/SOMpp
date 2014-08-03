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

#include <iostream>
#include <string.h>

#include <sys/mman.h>

#include "PagedHeap.h"
#include "Page.h"
#include "../vmobjects/VMObject.h"

/*
#if GC_TYPE==PAUSELESS
#define PG_HEAP(exp)
#else
#define PG_HEAP(exp) (_HEAP->exp)
#endif */

HEAP_CLS* PagedHeap::theHeap = NULL;

void PagedHeap::InitializeHeap(long objectSpaceSize, long pageSize) {
    if (theHeap) {
        cout << "Warning, reinitializing already initialized Heap, "
                << "all data will be lost!" << endl;
        delete theHeap;
    }
    theHeap = new HEAP_CLS(objectSpaceSize, pageSize);
}

void PagedHeap::DestroyHeap() {
    if (theHeap)
        delete theHeap;
}

PagedHeap::PagedHeap(long objectSpaceSize, long pageSize) {
    this->pageSize = pageSize;

#if GC_TYPE==PAUSELESS
    pthread_mutex_init(&numberOfMarkedRootSetsMutex, NULL);
#endif
    pthread_mutex_init(&pageRequestMutex, NULL);
    pthread_mutex_init(&fullPagesMutex, NULL);
    allPages = new vector<Page*>();
    availablePages = new vector<Page*>();
    fullPages = new vector<Page*>();
    // create region in which we can allocate objects, pages will be created in this region
    memoryStart = mmap(NULL, objectSpaceSize, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, 0, 0);
    memset(memoryStart, 0x0, objectSpaceSize);
    memoryEnd = (size_t)memoryStart + objectSpaceSize;
    // initialize some meta data of the heap
    maxObjSize = pageSize / 2;
    nextFreePagePosition = memoryStart;
    collectionLimit = (void*)((size_t)memoryStart + ((size_t)(objectSpaceSize * 0.9)));
}

PagedHeap::~PagedHeap() {
    delete gc;
}

size_t PagedHeap::GetMaxObjectSize() {
    return maxObjSize;
}

Page* PagedHeap::RequestPage() {
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

void PagedHeap::RelinquishPage(Page* page) {
    pthread_mutex_lock(&pageRequestMutex);
    availablePages->push_back(page);
    pthread_mutex_unlock(&pageRequestMutex);
}

void PagedHeap::RelinquishFullPage(Page* page) {
    pthread_mutex_lock(&fullPagesMutex);
    fullPages->push_back(page);
    pthread_mutex_unlock(&fullPagesMutex);
}


#if GC_TYPE==PAUSELESS
void PagedHeap::IncrementMarkedRootSets() {
    pthread_mutex_lock(&numberOfMarkedRootSetsMutex);
    numberOfMarkedRootSets++;
    pthread_mutex_unlock(&numberOfMarkedRootSetsMutex);
}
#endif
