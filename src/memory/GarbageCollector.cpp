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
#include <map>
#include <string.h>

#include "GarbageCollector.h"
#include "Heap.h"

#include "../vm/Universe.h"

#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMObject.h"
#include "../vmobjects/VMFreeObject.h"
#include "../vmobjects/VMSymbol.h"


GarbageCollector::GarbageCollector(Heap* h) {
	heap = h;
    numCollections = 0;
	numLive = 0;
	spcLive = 0;
	numFreed = 0;
	spcFreed = 0;
    
}


GarbageCollector::~GarbageCollector() {
    //Heap is deleted by Universe
}


void GarbageCollector::Collect() {
    ++numCollections;
	numLive = 0;
	spcLive = 0;
	numFreed = 0;
	spcFreed = 0;
	markReachableObjects();
	void* pointer = heap->objectSpace;
    VMFreeObject* lastUnusedObject = NULL;
	long bytesToSkip = 0;

    //reset freeList, will be rebuilt during sweeping
    heap->freeListStart = NULL;

    //start sweeping
    //iterate through the whole heap
    do { //end of heap not reached yet?
        //everything in the heap is an vmobject
        VMObject* curObject = (VMObject*) pointer;
        if (curObject->GetGCField() != -1) {
            //current object is not marked as being unused
            if (curObject->GetGCField() == 1) {
                //found alive object
                ++numLive;
                spcLive += curObject->GetObjectSize();
                curObject->SetGCField(0);
            } else {
                //found trash
                ++numFreed;
                int freedBytes = curObject->GetObjectSize();
                spcFreed += freedBytes;
                memset(curObject, 0, freedBytes);

                //mark object as unused
                curObject->SetGCField(-1);
                curObject->SetObjectSize(freedBytes);
                VMFreeObject* curFree = (VMFreeObject*) curObject;
                if (heap->freeListStart == NULL) {
                    heap->freeListStart = curFree;
                } else  {
                    lastUnusedObject->SetNext(curFree);
                }
                curFree->SetPrevious(lastUnusedObject);
                lastUnusedObject = curFree;
                
            }
        } else {
            VMFreeObject* curFree = (VMFreeObject*)curObject;
            //store the unused object for merging purposes
            if (heap->freeListStart == NULL) 
                heap->freeListStart = curFree;
            else
                lastUnusedObject->SetNext(curFree);
            curFree->SetPrevious(lastUnusedObject);
            lastUnusedObject = curFree;
        }
        pointer = (void*)((long)pointer + curObject->GetObjectSize());
    }while((long)pointer  < ((long)(void*)heap->objectSpace) + heap->objectSpaceSize);
    
    mergeFreeSpaces();

    if(gcVerbosity > 1)
        this->PrintCollectStat();
    if(gcVerbosity > 2) {
        cerr << "TODO: dump heap" << endl;
    }
}


void GarbageCollector::markReachableObjects() {
	map<pVMSymbol, pVMObject> globals = Universe::GetUniverse()->GetGlobals();
    for (map<pVMSymbol, pVMObject>::iterator it = globals.begin(); 
                                        it!= globals.end(); ++it) {
        (&(*it->first))->MarkReferences();

        //The NULL check for the second entry is necessary because for
        //some reason the True, False, Boolean, and System classes
        //get into the globals map although they shouldn't. Although 
        //I tried to find out why, I never did... :( They are not entered
        //into the map using Universe::SetGlobal and there is no other way
        //to enter them into that map....
        if (&(*it->second) != NULL) (&(*it->second))->MarkReferences();
	}
    // Get the current frame and mark it.
	// Since marking is done recursively, this automatically
	// marks the whole stack
    pVMFrame currentFrame = _UNIVERSE->GetInterpreter()->GetFrame();
    if (currentFrame != NULL) {
        ((pVMObject)currentFrame)->MarkReferences();
    }
}

void GarbageCollector::mergeFreeSpaces() {

	VMFreeObject* currentEntry = heap->freeListStart;
    VMFreeObject* last = NULL;
	heap->sizeOfFreeHeap = 0;
	while (currentEntry->GetNext() != NULL) {
		if((int)currentEntry + (int)currentEntry->GetObjectSize() == 
                                        (int)currentEntry->GetNext()) {
            int newEntrySize = currentEntry->GetObjectSize() +
                                        currentEntry->GetNext()->GetObjectSize();
			currentEntry->SetObjectSize(newEntrySize);
			currentEntry->SetNext(currentEntry->GetNext()->GetNext());
		} else {
			heap->sizeOfFreeHeap += currentEntry->GetObjectSize();
            currentEntry->SetPrevious(last);
            last = currentEntry;
			currentEntry = currentEntry->GetNext();
		}
	}
    currentEntry->SetPrevious(last);
	heap->sizeOfFreeHeap += currentEntry->GetObjectSize();
    
}

#define _KB(B) (B/1024)
#define _MB(B) ((double)B/(1024.0*1024.0))

void GarbageCollector::PrintGCStat() const {
    cerr << "-- GC statistics --" << endl;
    cerr << "* heap size " << heap->objectSpaceSize << " B (" << 
        _KB(heap->objectSpaceSize) << " kB, " << 
        _MB(heap->objectSpaceSize) << " MB)" << endl;
    cerr << "* performed " << numCollections << " collections" << endl;
}

void GarbageCollector::PrintCollectStat() const {
    cerr << endl << "[GC " << numCollections << ", " << 
        heap->numAlloc << " alloc (" << _KB(heap->spcAlloc) <<
        " kB), " << numLive << " live (" << _KB(spcLive) <<
        " kB), " << numFreed << " freed (" <<  _KB(spcFreed) <<
        " kB)]" << endl;
}

