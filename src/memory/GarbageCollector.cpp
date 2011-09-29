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
#ifdef USE_TAGGING
#include "../vmobjects/VMIntPointer.h"
#endif
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMObject.h"
#include "../vmobjects/VMSymbol.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMBlock.h"
#include "../vmobjects/VMPrimitive.h"
#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMEvaluationPrimitive.h"

#define _KB(B) (B/1024)
#define _MB(B) ((double)B/(1024.0*1024.0))
#define INITIAL_MAJOR_COLLECTION_THRESHOLD (5 * 1024 * 1024) //5 MB


GarbageCollector::GarbageCollector(Heap* h) {
	heap = h;
	majorCollectionThreshold = INITIAL_MAJOR_COLLECTION_THRESHOLD; 
	matureObjectsSize = 0;
	}

GarbageCollector::~GarbageCollector() {
    //Heap is deleted by Universe
}


#ifdef USE_TAGGING
AbstractVMObject* copy_if_necessary(AbstractVMObject* obj) {
	//don't process tagged objects
	if ((int32_t)((void*)obj) & 0x1)
		return obj;
#else
pVMObject copy_if_necessary(pVMObject obj) {
#endif

	int32_t gcField = obj->GetGCField();
	//if this is an old object already, we don't have to copy
	if (gcField & MASK_OBJECT_IS_OLD)
		return obj;
	//GCField is abused as forwarding pointer here
	//if someone has moved before, return the moved object
	if (gcField != 0)
#ifdef USE_TAGGING
		return (AbstractVMObject*)gcField;
#else
		return (pVMObject)gcField;
#endif
	//we have to clone ourselves
#ifdef USE_TAGGING
	AbstractVMObject* newObj = obj->Clone();
#else
	pVMObject newObj = obj->Clone();
#endif
	obj->SetGCField((int32_t)newObj);
	newObj->SetGCField(MASK_OBJECT_IS_OLD);
    //walk recursively
    newObj->WalkObjects(copy_if_necessary);
	return newObj;
}

#ifdef USE_TAGGING
AbstractVMObject* mark_object(AbstractVMObject* obj) {
	if ((int32_t)((void*)obj) & 1)
		return obj;
#else
pVMObject mark_object(pVMObject obj) {
#endif
    if (obj->GetGCField() & MASK_OBJECT_IS_MARKED)
        return (obj);
    obj->SetGCField(MASK_OBJECT_IS_OLD | MASK_OBJECT_IS_MARKED);
    obj->WalkObjects(&mark_object);
    return obj;
}

void GarbageCollector::MinorCollection() {
    //walk all globals
    _UNIVERSE->WalkGlobals(&copy_if_necessary);
    //and the current frame
    pVMFrame currentFrame = _UNIVERSE->GetInterpreter()->GetFrame();
    if (currentFrame != NULL) {
#ifdef USE_TAGGING
        pVMFrame newFrame =	(VMFrame*)copy_if_necessary(currentFrame.GetPointer());
#else
        pVMFrame newFrame = (pVMFrame)copy_if_necessary(currentFrame);
#endif
		assert(_HEAP->isObjectInNursery(newFrame) == false);
        _UNIVERSE->GetInterpreter()->SetFrame(newFrame);
    }

    //and also all objects that have been detected by the write barriers
    for (vector<int>::iterator objIter =
			_HEAP->oldObjsWithRefToYoungObjs->begin(); objIter !=
			_HEAP->oldObjsWithRefToYoungObjs->end(); objIter++) {
		//content of oldObjsWithRefToYoungObjs is not altered while iteration,
		// because copy_if_necessary returns old objs only -> ignored by
		// write_barrier
#ifdef USE_TAGGING
		AbstractVMObject* obj = (AbstractVMObject*)(*objIter);
#else
		pVMObject obj = (pVMObject)(*objIter);
#endif
		obj->SetGCField(MASK_OBJECT_IS_OLD);
        obj->WalkObjects(&copy_if_necessary);
	}
	_HEAP->oldObjsWithRefToYoungObjs->clear();
	_HEAP->nextFreePosition = _HEAP->nursery;
}

void GarbageCollector::MajorCollection() {
    //first we have to mark all objects (globals and current frame recursively)
    _UNIVERSE->WalkGlobals(&mark_object);
    //and the current frame
    pVMFrame currentFrame = _UNIVERSE->GetInterpreter()->GetFrame();
    if (currentFrame != NULL) {
#ifdef USE_TAGGING
        VMFrame* newFrame = (VMFrame*)mark_object(currentFrame);
#else
        pVMFrame newFrame = (pVMFrame)mark_object(currentFrame);
#endif
        _UNIVERSE->GetInterpreter()->SetFrame(newFrame);
    }

    //now that all objects are marked we can safely delete all allocated objects that are not marked
    vector<pVMObject>* survivors = new vector<pVMObject>();
	for (vector<pVMObject>::iterator objIter =
			_HEAP->allocatedObjects->begin(); objIter !=
			_HEAP->allocatedObjects->end(); objIter++) {
		if ((*objIter)->GetGCField() & MASK_OBJECT_IS_MARKED) {
			survivors->push_back(*objIter);
			(*objIter)->SetGCField(MASK_OBJECT_IS_OLD);
		}
		else {
			_HEAP->FreeObject(*objIter);
		}
    }
	delete _HEAP->allocatedObjects;
	_HEAP->allocatedObjects = survivors;
}


void GarbageCollector::Collect() {
	Timer::GCTimer->Resume();
	//reset collection trigger
    heap->gcTriggered = false;

    MinorCollection();
    if (_HEAP->matureObjectsSize > majorCollectionThreshold)
	{
		MajorCollection();
		majorCollectionThreshold = 2 * _HEAP->matureObjectsSize;

	}
	Timer::GCTimer->Halt();
}


