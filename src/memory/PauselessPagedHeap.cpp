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
#include <misc/debug.h>

#include <sys/mman.h>

#include <memory/PauselessPagedHeap.h>
#include "PauselessPage.h"

#include <vmobjects/VMObject.h>

HEAP_CLS* PauselessPagedHeap::theHeap = nullptr;


PauselessPagedHeap::PauselessPagedHeap(size_t pageSize, size_t maxHeapSize)
    : Heap<PauselessHeap>(nullptr) {
    pthread_mutex_init(&fullPagesMutex, nullptr);
    allPages = new vector<PauselessPage*>();
    availablePages = new vector<PauselessPage*>();
    fullPages = new vector<PauselessPage*>();
    memoryStart = mmap(nullptr, maxHeapSize, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, 0, 0);
    memset(memoryStart, 0x0, maxHeapSize);
    memoryEnd = (size_t)memoryStart + maxHeapSize;
    maxObjSize = PAGE_SIZE / 2;
    CreatePages();
}

void PauselessPagedHeap::CreatePages() {
    void* nextFreePagePosition = memoryStart;
    PauselessPage* newPage;
    while (nextFreePagePosition < (void*) memoryEnd) {
        newPage = new PauselessPage(nextFreePagePosition, static_cast<PauselessHeap*>(this));
        allPages->push_back(newPage);
        availablePages->push_back(newPage);
        nextFreePagePosition = (void*) ((size_t)nextFreePagePosition + PAGE_SIZE);
    }
}

PauselessPagedHeap::~PauselessPagedHeap() {
    delete gc;
}

size_t PauselessPagedHeap::GetMaxObjectSize() {
    return maxObjSize;
}

PauselessPage* PauselessPagedHeap::RequestPage() {
    lock_guard<mutex> lock(availablePages_mutex);
    
    if (availablePages->empty())
        GetUniverse()->ErrorExit("Unable to respond to page request");
    PauselessPage* newPage = availablePages->back();
    availablePages->pop_back();
    checkCollectionTreshold();
    
    return newPage;
}

void PauselessPagedHeap::RelinquishPage(PauselessPage* page) {
    pthread_mutex_lock(&fullPagesMutex);
    fullPages->push_back(page);
    pthread_mutex_unlock(&fullPagesMutex);
}

void PauselessPagedHeap::AddEmptyPage(PauselessPage* page) {
    lock_guard<mutex> lock(availablePages_mutex);
    
    availablePages->push_back(page);
}
