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
#include <cstdlib>

#include "GarbageCollector.h"
#include "Heap.h"

#include "../vm/Universe.h"

#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMObject.h"
#include "../vmobjects/VMSymbol.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMBlock.h"
#include "../vmobjects/VMPrimitive.h"
#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMEvaluationPrimitive.h"

#define GC_MARKED 3456

GarbageCollector::GarbageCollector(Heap* h) {
	heap = h;
}


GarbageCollector::~GarbageCollector() {
    //Heap is deleted by Universe
}


map<pVMObject, pVMObject> movedObjects;

void moveSome(vector<pVMObject>* allocatedObjects) {
	movedObjects.clear();
	int32_t noObjects = allocatedObjects->size();
	for (int32_t i = 0; i < min(100, noObjects); i++) {
		pVMObject obj = (*allocatedObjects)[i];
		pVMObject clone = obj->Clone();
		movedObjects.insert(pair<pVMObject, pVMObject>(obj,clone));
	}
}


void GarbageCollector::Collect() {
	//reset collection trigger
	heap->gcTriggered = false;

	//move some objects
	moveSome(heap->allocatedObjects);
	//now mark all reachables
	markReachableObjects();

	//in this survivors stack we will remember all objects that survived
	vector<pVMObject>* survivors = new vector<pVMObject>();
	int32_t survivorsSize = 0;

	vector<pVMObject>::iterator iter;
	for (iter = heap->allocatedObjects->begin(); iter !=
			heap->allocatedObjects->end(); iter++) {
		if ((*iter)->GetGCField() == GC_MARKED) {
			//object ist marked -> let it survive
			survivors->push_back(*iter);
			survivorsSize += (*iter)->GetObjectSize();
			(*iter)->SetGCField(0);
		} else {
			//not marked -> kill it
			heap->FreeObject(*iter);
		}
	}

	heap->allocatedObjects = survivors;

	heap->spcAlloc = survivorsSize;
	//TODO: Maybe choose another constant to calculate new collectionLimit here
	heap->collectionLimit = 2 * survivorsSize;
}

pVMObject markObject(pVMObject obj) {
    map<pVMObject, pVMObject>::iterator moIter = movedObjects.find(obj);
    if (moIter != movedObjects.end())
	    obj = moIter->second;
    if (obj->GetGCField())
        return obj;

    obj->SetGCField(GC_MARKED);
    obj->WalkObjects(markObject);
    return obj;
}


void GarbageCollector::markReachableObjects() {
	_UNIVERSE->WalkGlobals(markObject);


    // Get the current frame and mark it.
	// Since marking is done recursively, this automatically
	// marks the whole stack
    pVMFrame currentFrame = _UNIVERSE->GetInterpreter()->GetFrame();
    if (currentFrame != NULL) {
		pVMFrame newFrame = (pVMFrame)markObject(currentFrame);
		_UNIVERSE->GetInterpreter()->SetFrame(newFrame);
    }
}

#define _KB(B) (B/1024)
#define _MB(B) ((double)B/(1024.0*1024.0))
