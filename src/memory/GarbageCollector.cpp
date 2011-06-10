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



pVMObject copy_if_necessary(pVMObject obj) {
	//GCField is abused as forwarding pointer here
	//if someone has moved before, return the moved object
	int32_t gcField = obj->GetGCField();
	if (gcField != 0)
		return (pVMObject)gcField;
	//we also don't have to copy, if we are not inside the nursery
	if (_HEAP->isObjectInNursery(obj) == false)
		return obj;
	//we have to clone ourselves
	pVMObject newObj = obj->Clone();
	newObj->SetGCField(0); //XXX: Shouldbe 0 anyway -->just for testing
	obj->SetGCField((int32_t)newObj);
    //walk recursively
    newObj->WalkObjects(copy_if_necessary);
	return newObj;
}

pVMObject mark_object(pVMObject obj) {
    if (obj->GetGCField() == GC_MARKED)
        return (obj);
    obj->SetGCField(GC_MARKED);
    obj->WalkObjects(&mark_object);
    return obj;
}

void GarbageCollector::MinorCollection() {
    //walk all globals
    _UNIVERSE->WalkGlobals(&copy_if_necessary);
    //and the current frame
    pVMFrame currentFrame = _UNIVERSE->GetInterpreter()->GetFrame();
    if (currentFrame != NULL) {
        pVMFrame newFrame = (pVMFrame)copy_if_necessary(currentFrame);
		assert(_HEAP->isObjectInNursery(newFrame) == false);
        _UNIVERSE->GetInterpreter()->SetFrame(newFrame);
    }

    //and also all objects that have been detected by the write barriers
    for (vector<int>::iterator objIter =
			_HEAP->oldObjsWithRefToYoungObjs->begin(); objIter !=
			_HEAP->oldObjsWithRefToYoungObjs->end(); objIter++)
		//content of oldObjsWithRefToYoungObjs is not altered while iteration,
		// because copy_if_necessary returns old objs only -> ignored by
		// write_barrier
        ((pVMObject)(*objIter))->WalkObjects(&copy_if_necessary);
	_HEAP->oldObjsWithRefToYoungObjs->clear();

    //clear the nursery now
    memset(_HEAP->nursery, 0xFF, _HEAP->nurserySize);
	_HEAP->nextFreePosition = _HEAP->nursery;
}

void GarbageCollector::MajorCollection() {
    //first we have to mark all objects (globals and current frame recursively)
    _UNIVERSE->WalkGlobals(&mark_object);
    //and the current frame
    pVMFrame currentFrame = _UNIVERSE->GetInterpreter()->GetFrame();
    if (currentFrame != NULL) {
        pVMFrame newFrame = (pVMFrame)mark_object(currentFrame);
        _UNIVERSE->GetInterpreter()->SetFrame(newFrame);
    }

    //now that all objects are marked we can safely delete all allocated objects that are not marked
    vector<pVMObject>* survivors = new vector<pVMObject>();
	for (vector<pVMObject>::iterator objIter =
			_HEAP->allocatedObjects->begin(); objIter !=
			_HEAP->allocatedObjects->end(); objIter++) {
		if ((*objIter)->GetGCField() == GC_MARKED) {
			survivors->push_back(*objIter);
			(*objIter)->SetGCField(0);
		}
		else {
			_HEAP->FreeObject(*objIter);
		}
    }
	delete _HEAP->allocatedObjects;
	_HEAP->allocatedObjects = survivors;
}


void GarbageCollector::Collect() {
	//reset collection trigger
    heap->gcTriggered = false;

    MinorCollection();
    if (true) //XXX need useful condition for major collection here
        MajorCollection();


}

#define _KB(B) (B/1024)
#define _MB(B) ((double)B/(1024.0*1024.0))
