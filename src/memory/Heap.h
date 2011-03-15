#pragma once
#ifndef HEAP_H_
#define HEAP_H_

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
#include <stack>

#include "GarbageCollector.h"
#include "../misc/defs.h"
#include "../vmobjects/ObjectFormats.h"

class AbstractVMObject;

//macro to access the heap
#define _HEAP Heap::GetHeap()

class Heap
{
	friend class GarbageCollector;

public:
    static Heap* GetHeap();
    static void InitializeHeap(int objectSpaceSize = 1048576);
    static void DestroyHeap();
	Heap(int objectSpaceSize = 1048576);
	~Heap();
    AbstractVMObject* AllocateObject(size_t size);
    void FreeObject(AbstractVMObject*);
	void triggerGC(void);
	bool isCollectionTriggered(void);
    void FullGC();
	
    
private:
    static Heap* theHeap;
    std::stack<pVMObject>* allocatedObjects;
    std::stack<pVMObject>* otherAllocatedObjects;
	//flag that shows if a Collection is triggered
	bool gcTriggered;
	GarbageCollector* gc;
    uint32_t spcAlloc;
    uint32_t collectionLimit;
};

#endif
