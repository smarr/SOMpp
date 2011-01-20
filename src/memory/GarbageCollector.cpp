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
#include <assert.h>

#include "GarbageCollector.h"
#include "Heap.h"

#include "../vm/Universe.h"

#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMObject.h"
#include "../vmobjects/VMSymbol.h"


GarbageCollector::GarbageCollector(Heap* h) {
	heap = h;
}


GarbageCollector::~GarbageCollector() {
    //Heap is deleted by Universe
}


void GarbageCollector::Collect() {
	//reset collection trigger
	heap->gcTriggered = false;

	markReachableObjects();

	//in this survivors stack we will remember all objects that survived
	stack<pVMObject>* survivors = heap->otherAllocatedObjects;
	int32_t survivorsSize = 0;
	while (heap->allocatedObjects->size() > 0) {
		pVMObject obj = heap->allocatedObjects->top();
		if (obj->GetGCField() == 3456) {
			survivors->push(obj);
			survivorsSize += obj->GetObjectSize();
			obj->SetGCField(0);
		}
		else {
			heap->FreeObject(obj);
		}
		heap->allocatedObjects->pop();
	}
	heap->otherAllocatedObjects =heap->allocatedObjects;
	heap->allocatedObjects = survivors;
	heap->spcAlloc = survivorsSize;
	//TODO: Maybe choose another constant to calculate new collectionLimit here
	heap->collectionLimit = 2 * survivorsSize;
}

pVMObject markObject(pVMObject obj) {
    if (obj->GetGCField())
        return (pVMObject) 0xdeadbeef;
    obj->SetGCField(3456);
    obj->WalkObjects(markObject);
    return  (pVMObject) 0xdeadbeef;
}


void GarbageCollector::markReachableObjects() {
	map<pVMSymbol, pVMObject> globals = Universe::GetUniverse()->GetGlobals();
    for (map<pVMSymbol, pVMObject>::iterator it = globals.begin(); 
                                        it!= globals.end(); ++it) {
        markObject((&(*it->first))); // XXX use result value

        //The NULL check for the second entry is necessary because for
        //some reason the True, False, Boolean, and System classes
        //get into the globals map although they shouldn't. Although 
        //I tried to find out why, I never did... :( They are not entered
        //into the map using Universe::SetGlobal and there is no other way
        //to enter them into that map....
        if (&(*it->second) != NULL) markObject(&(*it->second));
	}
    // Get the current frame and mark it.
	// Since marking is done recursively, this automatically
	// marks the whole stack
    pVMFrame currentFrame = _UNIVERSE->GetInterpreter()->GetFrame();
    if (currentFrame != NULL) {
        markObject(((pVMObject)currentFrame));
    }
}

#define _KB(B) (B/1024)
#define _MB(B) ((double)B/(1024.0*1024.0))
