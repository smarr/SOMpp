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
    this->gcTriggered = false;
    this->threadCount = 0;
    this->readyForGCThreads = 0;
    pthread_mutex_init(&doCollect, NULL);
    pthread_mutex_init(&threadCountMutex, NULL);
    pthread_cond_init(&stopTheWorldCondition, NULL);
    pthread_cond_init(&mayProceed, NULL);
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
    if (availablePages->empty()) {
        newPage = new Page(nextFreePagePosition, this);
        assert(newPage);
        allPages->push_back(newPage);
        nextFreePagePosition = (void*) ((size_t)nextFreePagePosition + pageSize);
        if (nextFreePagePosition > collectionLimit)
            triggerGC();
    } else {
        newPage = availablePages->back();
        availablePages->pop_back();
    }
    return newPage;
}

void PagedHeap::RelinquishPage(Page* page) {
    availablePages->push_back(page);
}

void PagedHeap::RelinquishFullPage(Page* page) {
    fullPages->push_back(page);
}

void PagedHeap::FullGC() {
    // one thread is going to do the GC
    if (pthread_mutex_trylock(&doCollect) == 0) {
        // all threads must have reached a safe point for the GC to take place
        pthread_mutex_lock(&threadCountMutex);
        while (threadCount != readyForGCThreads) {
            pthread_cond_wait(&stopTheWorldCondition, &threadCountMutex);
        }
        pthread_mutex_unlock(&threadCountMutex);
        // all threads have reached a safe point
        gc->Collect();
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

void PagedHeap::IncrementThreadCount() {
    pthread_mutex_lock(&threadCountMutex);
    threadCount++;
    pthread_mutex_unlock(&threadCountMutex);
}

void PagedHeap::DecrementThreadCount() {
    pthread_mutex_lock(&threadCountMutex);
    threadCount--;
    pthread_mutex_unlock(&threadCountMutex);
}

void PagedHeap::IncrementWaitingForGCThreads() {
    pthread_mutex_lock(&threadCountMutex);
    readyForGCThreads++;
    pthread_cond_signal(&stopTheWorldCondition);
    pthread_mutex_unlock(&threadCountMutex);
}

void PagedHeap::DecrementWaitingForGCThreads() {
    pthread_mutex_lock(&threadCountMutex);
    readyForGCThreads--;
    pthread_cond_signal(&stopTheWorldCondition);
    pthread_mutex_unlock(&threadCountMutex);
}
