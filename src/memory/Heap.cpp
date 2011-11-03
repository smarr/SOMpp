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
#include "../vmobjects/VMObject.h"
#include "../vm/Universe.h"

HEAP_CLS* Heap::theHeap = NULL;

bool Heap::isCollectionTriggered(void) {
	return gcTriggered;
}

void Heap::InitializeHeap( int objectSpaceSize ) {
    if (theHeap) {
        cout << "Warning, reinitializing already initialized Heap, " 
             << "all data will be lost!" << endl;
        delete theHeap;
    }
    theHeap = new HEAP_CLS(objectSpaceSize);
}

void Heap::DestroyHeap() {
    if (theHeap) delete theHeap;
}

Heap::Heap(int objectSpaceSize) {
}

Heap::~Heap() {
	delete gc;
}

void Heap::FullGC() {
    gc->Collect();
}
