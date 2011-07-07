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

	nursery = malloc(objectSpaceSize);
	nurserySize = objectSpaceSize;
	maxNurseryObjSize = objectSpaceSize / 2;
	nursery_end = (int32_t)nursery + nurserySize;
	matureObjectsSize = 0;
	memset(nursery, 0x0, objectSpaceSize);
	collectionLimit = (void*)((int32_t)nursery + ((int32_t)(objectSpaceSize *
					0.9)));
	nextFreePosition = nursery;
	allocatedObjects = new vector<pVMObject>();
	oldObjsWithRefToYoungObjs = new vector<int>();
}
void Heap::writeBarrier_OldHolder(pVMObject holder, const pVMObject
		referencedObject) {
	if (isObjectInNursery(referencedObject)
			&& ((holder->GetGCField() & MASK_SEEN_BY_WRITE_BARRIER) ==false)) {
		oldObjsWithRefToYoungObjs->push_back((int32_t)holder);
		holder->SetGCField(holder->GetGCField() | MASK_SEEN_BY_WRITE_BARRIER);
	}
}

Heap::~Heap() {
	delete gc;
}

void Heap::FreeObject(pVMObject obj) {
	matureObjectsSize -= obj->GetObjectSize();
	delete obj;
}

AbstractVMObject* Heap::AllocateNurseryObject(size_t size) {
	size_t paddedSize = size + PAD_BYTES(size);
	AbstractVMObject* newObject = (AbstractVMObject*) nextFreePosition;
	nextFreePosition = (void*)((int32_t)nextFreePosition + (int32_t)paddedSize);
	if ((int32_t)nextFreePosition > (int32_t)nursery + nurserySize) {
		cout << "Failed to allocate " << size << " Bytes in nursery." << endl;
		_UNIVERSE->Quit(-1);
	}
	//let's see if we have to trigger the GC
	if (nextFreePosition > collectionLimit)
		triggerGC();
	return newObject;
}

AbstractVMObject* Heap::AllocateMatureObject(size_t size) {
	size_t paddedSize = size + PAD_BYTES(size);
	pVMObject newObject = (pVMObject)malloc(paddedSize);
	if (newObject == NULL) {
		cout << "Failed to allocate " << size << " Bytes." << endl;
		_UNIVERSE->Quit(-1);
	}
	allocatedObjects->push_back(newObject);
	matureObjectsSize += paddedSize;
	return newObject;
}

void Heap::FullGC() {
    gc->Collect();
}
