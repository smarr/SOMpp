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

#include "Heap.h"
#include <vmobjects/VMObject.h>
#include <vm/Universe.h>

#include <vm/SafePoint.h>

template<class HEAP_T>
HEAP_T* Heap<HEAP_T>::theHeap = nullptr;

template<class HEAP_T>
void Heap<HEAP_T>::InitializeHeap(size_t pageSize, size_t objectSpaceSize) {
    if (theHeap) {
        if (!UNITTESTS) {
            Universe::ErrorPrint("Warning, reinitializing already initialized "
                                 "Heap, all data will be lost!\n");
        }
        delete theHeap;
    }
    theHeap = new HEAP_CLS(pageSize, objectSpaceSize);
}

template<class HEAP_T>
void Heap<HEAP_T>::DestroyHeap() {
    if (theHeap)
        delete theHeap;
}

template<class HEAP_T>
Heap<HEAP_T>::~Heap() {
    delete gc;
}

template<class HEAP_T>
void Heap<HEAP_T>::FullGC() {
    SafePoint::ReachSafePoint([this]() { gc->Collect(); });
}

template<class HEAP_T>
void Heap<HEAP_T>::FailedAllocation(size_t size) {
    GetUniverse()->ErrorExit("Failed to allocate " + to_string(size) + " Bytes.\n");
}

template<class HEAP_T>
void Heap<HEAP_T>::ReachedMaxNumberOfPages() {
    GetUniverse()->ErrorExit("Can't allocate more memory pages. Already reached maximum.");
}

template<class HEAP_T>
void Heap<HEAP_T>::ReportGCDetails() {
    Universe::ErrorPrint("Time spent in GC: [" +
                         to_string(Timer::GCTimer->GetTotalTime()) +
                         "] msec\n");
    
    // The pauseless heap was counting number of cycles performed
    // the generational probably wants to distinguish between full and
    // only minor GCs
    GetUniverse()->Print("TODO: implement some statistics for GCs");
}

// Instantitate Template for the heap classes
template void Heap<HEAP_CLS>::InitializeHeap(size_t, size_t);
template void Heap<HEAP_CLS>::DestroyHeap();
template void Heap<HEAP_CLS>::FullGC();
template void Heap<HEAP_CLS>::ReportGCDetails();

class GenerationalHeap;
template GenerationalHeap* Heap<GenerationalHeap>::theHeap;
template Heap<GenerationalHeap>::~Heap();
template void Heap<GenerationalHeap>::FailedAllocation(size_t);
template void Heap<GenerationalHeap>::ReachedMaxNumberOfPages();

class CopyingHeap;
template CopyingHeap* Heap<CopyingHeap>::theHeap;
template Heap<CopyingHeap>::~Heap();
template void Heap<CopyingHeap>::FailedAllocation(size_t);
template void Heap<CopyingHeap>::ReachedMaxNumberOfPages();

class MarkSweepHeap;
template MarkSweepHeap* Heap<MarkSweepHeap>::theHeap;
template Heap<MarkSweepHeap>::~Heap();
template void Heap<MarkSweepHeap>::FailedAllocation(size_t);
template void Heap<MarkSweepHeap>::ReachedMaxNumberOfPages();

