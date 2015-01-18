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

#include "PagedHeap.h"
#include "Page.h"
#include "../vmobjects/VMObject.h"

HEAP_CLS* PagedHeap::theHeap = nullptr;

void PagedHeap::InitializeHeap(long, long) {
    if (theHeap) {
        sync_out(ostringstream() << "Warning, reinitializing already initialized Heap, "
                << "all data will be lost!");
        delete theHeap;
    }
    theHeap = new HEAP_CLS(HEAP_SIZE, PAGE_SIZE);
}

void PagedHeap::DestroyHeap() {
    if (theHeap)
        delete theHeap;
}

PagedHeap::PagedHeap(long, long) {
    pthread_mutex_init(&fullPagesMutex, nullptr);
    pthread_mutex_init(&availablePagesMutex, nullptr);
    allPages = new vector<Page*>();
    availablePages = new vector<Page*>();
    fullPages = new vector<Page*>();
    memoryStart = mmap(nullptr, HEAP_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, 0, 0);
    memset(memoryStart, 0x0, HEAP_SIZE);
    memoryEnd = (size_t)memoryStart + HEAP_SIZE;
    maxObjSize = PAGE_SIZE / 2;
    CreatePages();
}

void PagedHeap::CreatePages() {
    void* nextFreePagePosition = memoryStart;
    Page* newPage;
    while (nextFreePagePosition < (void*) memoryEnd) {
        newPage = new Page(nextFreePagePosition, this);
        allPages->push_back(newPage);
        availablePages->push_back(newPage);
        nextFreePagePosition = (void*) ((size_t)nextFreePagePosition + PAGE_SIZE);
    }
}

PagedHeap::~PagedHeap() {
    delete gc;
}

size_t PagedHeap::GetMaxObjectSize() {
    return maxObjSize;
}

Page* PagedHeap::RequestPage() {
    pthread_mutex_lock(&availablePagesMutex);
    if (availablePages->empty())
        GetUniverse()->ErrorExit("Unable to respond to page request");
    Page* newPage = availablePages->back();
    availablePages->pop_back();
    checkCollectionTreshold();
    pthread_mutex_unlock(&availablePagesMutex);
    return newPage;
}

void PagedHeap::RelinquishPage(Page* page) {
    pthread_mutex_lock(&fullPagesMutex);
    fullPages->push_back(page);
    pthread_mutex_unlock(&fullPagesMutex);
}

void PagedHeap::AddEmptyPage(Page* page) {
    pthread_mutex_lock(&availablePagesMutex);
    availablePages->push_back(page);
    pthread_mutex_unlock(&availablePagesMutex);
}

int PagedHeap::GetNumberOfCycles() {
    return gc->GetNumberOfCycles();
}