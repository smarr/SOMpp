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
	if (gcField != 0) {
		return (pVMObject)gcField;}
	//we have to clone ourselves
	pVMObject newObj = obj->Clone();
	obj->SetGCField((int32_t)newObj);
	return newObj;
}

void GarbageCollector::Collect() {
	//reset collection trigger
	heap->gcTriggered = false;

	_HEAP->switchBuffers();
	_UNIVERSE->WalkGlobals(copy_if_necessary);
        pVMFrame currentFrame = _UNIVERSE->GetInterpreter()->GetFrame();
        if (currentFrame != NULL) {
  		pVMFrame newFrame = (pVMFrame)copy_if_necessary(currentFrame);
		_UNIVERSE->GetInterpreter()->SetFrame(newFrame);
        }

	//now copy all objects that are referenced by the objects we have moved so far
	pVMObject curObject = (pVMObject)(_HEAP->currentBuffer);
	while (curObject < _HEAP->nextFreePosition) {
		curObject->WalkObjects(copy_if_necessary);
		//assert(dynamic_cast<pVMObject>(curObject) != NULL);
		curObject = (pVMObject)((int32_t)curObject + curObject->GetObjectSize());
	}

	//clear the old buffer now
	memset(_HEAP->oldBuffer, 0x0, (int32_t)(_HEAP->currentBufferEnd) - (int32_t)(_HEAP->currentBuffer));
}

#define _KB(B) (B/1024)
#define _MB(B) ((double)B/(1024.0*1024.0))
