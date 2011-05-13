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
#include <stdlib.h>
#include <string.h>

#include "Heap.h"
#include "../vmobjects/VMObject.h"
#include "../vm/Universe.h"

Heap* Heap::theHeap = NULL;

Heap* Heap::GetHeap() {
    if (!theHeap) {
        _UNIVERSE->ErrorExit("Trying to access uninitialized Heap");
    }
    return theHeap;
}

void Heap::triggerGC(void) {
	gcTriggered = true;
}

void Heap::addToList(const pVMObject obj) {
    oldObjsWithRefToYoungObjs.push_back(obj);    
}

bool Heap::isCollectionTriggered(void) {
	return gcTriggered;
}

void Heap::InitializeHeap( int objectSpaceSize ) {
    if (theHeap) {
        cout << "Warning, reinitializing already initialized Heap, " 
             << "all data will be lost!" << endl;
        delete theHeap;
    }
    theHeap = new Heap(objectSpaceSize);
}

void Heap::DestroyHeap() {
    if (theHeap) delete theHeap;
}

Heap::Heap(int objectSpaceSize) {
	//our initial collection limit is 90% of objectSpaceSize
	//collectionLimit = objectSpaceSize * 0.9;
	gc = new GarbageCollector(this);

	size_t bufSize = objectSpaceSize;
	buffers[0] = malloc(bufSize);
	buffers[1] = malloc(bufSize);
	currentBuffer = buffers[0];
	oldBuffer = buffers[1];
	memset(currentBuffer, 0x0, bufSize);
	memset(oldBuffer, 0x0, bufSize);
	currentBufferEnd = (void*)((int32_t)currentBuffer + bufSize);
	collectionLimit = (void*)((int32_t)currentBuffer + ((int32_t)(bufSize *
					0.9)));
	nextFreePosition = currentBuffer;
}

Heap::~Heap() {
	delete gc;
}

void Heap::switchBuffers() {
	int32_t bufSize = (int32_t)currentBufferEnd - (int32_t)currentBuffer;
	if (currentBuffer == buffers[0]) {
		currentBuffer = buffers[1];
		oldBuffer = buffers[0];
	}
	else {
		currentBuffer = buffers[0];
		oldBuffer = buffers[1];
	}
	currentBufferEnd = (void*)((int32_t)currentBuffer + bufSize);
	nextFreePosition = currentBuffer;
	collectionLimit = (void*)((int32_t)currentBuffer + (int32_t)(0.9 *
				bufSize));
}

AbstractVMObject* Heap::AllocateObject(size_t size) {
	size_t paddedSize = size + PAD_BYTES(size);
	AbstractVMObject* newObject = (AbstractVMObject*) nextFreePosition;
	nextFreePosition = (void*)((int32_t)nextFreePosition + (int32_t)paddedSize);
	if (nextFreePosition > currentBufferEnd) {
		cout << "Failed to allocate " << size << " Bytes." << endl;
		_UNIVERSE->Quit(-1);
	}
	//let's see if we have to trigger the GC
	if (nextFreePosition > collectionLimit)
		triggerGC();
	return newObject;
}

void Heap::FullGC() {
    gc->Collect();
}
