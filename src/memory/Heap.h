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

#include <vector>
#include <set>
#include <cstdlib>
#include "GarbageCollector.h"
#include "../misc/defs.h"
#include "../vmobjects/ObjectFormats.h"

using namespace std;

template<class HEAP_T>
class Heap {
    friend class GarbageCollector<HEAP_T>;

public:
    static void InitializeHeap(long objectSpaceSize);
    static void DestroyHeap();
    Heap(GarbageCollector<HEAP_T>* const gc, long objectSpaceSize) : gc(gc), gcTriggered(false) {}
    ~Heap();
    inline void triggerGC()      { gcTriggered = true; }
    inline void resetGCTrigger() { gcTriggered = false; }
    bool isCollectionTriggered() { return gcTriggered;  }
    void FullGC();
    inline void FreeObject(AbstractVMObject* o) { free(o); }
protected:
    GarbageCollector<HEAP_T>* const gc;
private:
    template<class HEAP_U> friend HEAP_U* GetHeap();
    static HEAP_T* theHeap;
    
    // flag that shows if a Collection is triggered
    bool gcTriggered;
};

template<class HEAP_T>
inline HEAP_T* GetHeap() __attribute__ ((always_inline));
template<class HEAP_T>
HEAP_T* GetHeap() {
    return Heap<HEAP_T>::theHeap;
}
