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
#include "../vmobjects/VMFreeObject.h"

#include "../vm/Universe.h"

/*
 * macro for padding - only word-aligned memory must be allocated
 */
#define PAD_BYTES(N) ((sizeof(void*) - ((N) % sizeof(void*))) % sizeof(void*))

Heap* Heap::theHeap = NULL;

Heap* Heap::GetHeap() {
    if (!theHeap) {
        _UNIVERSE->ErrorExit("Trying to access uninitialized Heap");
    }
    return theHeap;
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
	objectSpace = malloc(objectSpaceSize);
	if (!objectSpace) {
		std::cout << "Failed to allocate the initial "<< objectSpaceSize 
                  << " bytes for the Heap. Panic.\n" << std::endl;
		exit(1);
	}
	memset(objectSpace, 0, objectSpaceSize);
	sizeOfFreeHeap = objectSpaceSize;
	this->objectSpaceSize = objectSpaceSize;
	this->buffersizeForUninterruptable = (int) (objectSpaceSize * 0.1);
    
    uninterruptableCounter = 0;
	numAlloc = 0;
    spcAlloc = 0;
    numAllocTotal = 0;
    freeListStart = (VMFreeObject*) objectSpace;
    freeListStart->SetObjectSize(objectSpaceSize);
    freeListStart->SetNext(NULL);
    freeListStart->SetPrevious(NULL);
    freeListStart->SetGCField(-1);
	/*freeListStart = (FreeListEntry*) objectSpace;
	freeListStart->size = objectSpaceSize;
	freeListStart->next = NULL;*/
	gc = new GarbageCollector(this);
}

Heap::~Heap() {
    if (gcVerbosity > 0) {
        cout << "-- Heap statistics --" << endl;
        cout << "Total number of allocations: " << numAllocTotal << endl;
        cout << "Number of allocations since last collection: " 
             << numAlloc << endl;
        std::streamsize p = cout.precision();
        cout.precision(3);
        cout << "Used memory: " << spcAlloc << "/" 
             << this->objectSpaceSize << " (" 
             << ((double)spcAlloc/(double)this->objectSpaceSize)*100 << "%)" << endl;
        cout.precision(p);
        gc->PrintGCStat();
    }
	free(objectSpace);
    
}

VMObject* Heap::AllocateObject(size_t size) {
    //add padding, so objects are word aligned
    size_t paddedSize = size + PAD_BYTES(size);
    VMObject* vmo = (VMObject*) Allocate(paddedSize);

    ++numAlloc;
    ++numAllocTotal;
    spcAlloc += paddedSize;
    return vmo;
}

void* Heap::Allocate(size_t size) {
	if (size == 0) return NULL;
    if (size < sizeof(VMObject))  {
        //this will never happen, as all allocation is done for VMObjects
        return internalAllocate(size);
    }
#ifdef HEAPDEBUG 
    std::cout << "allocating: " << (int)size << "bytes" << std::endl;
#endif
    //if there is not enough free heap size and we are not inside an uninterruptable
    //section of allocation, start garbage collection
	if (sizeOfFreeHeap <= buffersizeForUninterruptable &&
		uninterruptableCounter <= 0)  {
#ifdef HEAPDEBUG
        cout << "Not enough free memory, only: " << sizeOfFreeHeap 
             << " bytes left." << endl
             << "Starting Garbage Collection" << endl;
#endif
		gc->Collect();
        
        //
        //reset allocation stats
        //
        numAlloc = 0;
        spcAlloc = 0;
	}
	
	VMObject* result = NULL;
    VMFreeObject* cur = freeListStart;
    VMFreeObject* last = NULL;
    while(cur->GetObjectSize() != size &&
          cur->GetObjectSize() < size+sizeof(VMObject) &&
          cur->GetNext() != NULL) {
              last = cur;
              cur = cur->GetNext();
    }
    if (cur->GetObjectSize() == size) {
        //perfect fit
        if (cur == freeListStart) {
            freeListStart = cur->GetNext();
            freeListStart->SetPrevious(NULL);
        } else {
            last->SetNext(cur->GetNext());
            cur->GetNext()->SetPrevious(last);
        }
        result = cur;
    } else if (cur->GetObjectSize() >= size + sizeof(VMFreeObject)) {
        //found an entry that is big enough
        int oldSize = cur->GetObjectSize();
        VMFreeObject* oldNext = cur->GetNext();
        result = cur;
        VMFreeObject* replaceEntry = (VMFreeObject*) ((int)cur + size);
        replaceEntry->SetObjectSize(oldSize - size);
        replaceEntry->SetGCField(-1);
        replaceEntry->SetNext(oldNext);
        if (cur == freeListStart) {
            freeListStart = replaceEntry;
            freeListStart->SetPrevious(NULL);
        } else {
            last->SetNext(replaceEntry);
            replaceEntry->SetPrevious(last);
        }
    } else {
        //problem... might lose data here
        cout << "Not enough heap, data loss is possible" << endl;
        gc->Collect();
        this->numAlloc = 0;
        this->spcAlloc = 0;
        result = (VMObject*)this->Allocate(size);
    }

    if (result == NULL) {
        cout << "alloc failed" << endl;
        PrintFreeList();
        _UNIVERSE->ErrorExit("Failed to allocate");
    }

    memset(result, 0, size);
    result->SetObjectSize(size);
    this->sizeOfFreeHeap -= size;

    return result;
    
}

void Heap::PrintFreeList() {
    VMFreeObject* curEntry = freeListStart;
    int i =0;
    while (curEntry != NULL) {
        cout << "i: " << curEntry->GetObjectSize() << endl;
        ++i;
        curEntry = curEntry->GetNext();
    }
}

void Heap::FullGC() {
    gc->Collect();
}

void Heap::Free(void* ptr) {
    if ( ((int)ptr < (int) this->objectSpace) &&
        ((int)ptr > (int) this->objectSpace + this->objectSpaceSize)) {
        internalFree(ptr);
    }
}

void Heap::Destroy(VMObject* _object) {
    
    int freedBytes = _object->GetObjectSize();
    memset(_object, 0, freedBytes);
    VMFreeObject* object = (VMFreeObject*) _object;

    //see if there's an adjoining unused object behind this object
    VMFreeObject* next = (VMFreeObject*)((int)object + (int)freedBytes);
    if (next->GetGCField() == -1) {
        //yes, there is, so we can join them
        object->SetObjectSize(next->GetObjectSize() + freedBytes);
        object->SetNext(next->GetNext());
        next->GetNext()->SetPrevious(object);
        VMFreeObject* previous = next->GetPrevious();
        object->SetPrevious(previous);
        memset(next, 0, next->GetObjectSize());
    } else {
        //no, there is not, so we just put the new unused object as the new freeListStart
        object->SetObjectSize(freedBytes);
        object->SetNext(freeListStart);
        freeListStart->SetPrevious(object);
        freeListStart = object;
    }
    //TODO: find a way to merge unused objects that are before this object
}

void Heap::internalFree(void* ptr) {
    free(ptr);
}

void* Heap::internalAllocate(size_t size) {
    if (size == 0) return NULL;
    void* result = malloc(size);
    if(!result) {
        cout << "Failed to allocate " << size << " Bytes." << endl;
        _UNIVERSE->Quit(-1);
    }
    memset(result, 0, size);
    return result;
}
