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
#include <set>
#include "GarbageCollector.h"
#include "../misc/defs.h"
#include "../vmobjects/ObjectFormats.h"

class AbstractVMObject;
using namespace std;
//macro to access the heap
#define _HEAP Heap::GetHeap()
#ifdef DEBUG
struct VMObjectCompare {
  bool operator() (pair<const pVMObject, const pVMObject> lhs, pair<const
		  pVMObject, const pVMObject> rhs) const
  {return (int32_t)lhs.first<(int32_t)rhs.first &&
	  (int32_t)lhs.second<(int32_t)rhs.second ;}
};
#endif

class Heap
{
	friend class GarbageCollector;

public:
    static Heap* GetHeap();
    static void InitializeHeap(int objectSpaceSize = 1048576);
    static void DestroyHeap();
	Heap(int objectSpaceSize = 1048576);
	~Heap();
    AbstractVMObject* AllocateNurseryObject(size_t size);
	AbstractVMObject* AllocateMatureObject(size_t size);
	void FreeObject(pVMObject obj);
	void triggerGC(void);
	bool isCollectionTriggered(void);
    void FullGC();
	void writeBarrier(const pVMObject holder, const pVMObject referencedObject);
	inline bool isObjectInNursery(const pVMObject obj);
#ifdef DEBUG
	std::set<pair<const pVMObject, const pVMObject>, VMObjectCompare > writeBarrierCalledOn;
#endif
private:
    static Heap* theHeap;
    void addToList(const pVMObject holder);


	//members for moving GC
	void* nursery;
	void* nextFreePosition;
	int32_t nurserySize;

	//flag that shows if a Collection is triggered
	bool gcTriggered;
	GarbageCollector* gc;
    void* collectionLimit;
	vector<int>* oldObjsWithRefToYoungObjs;
	vector<pVMObject>* allocatedObjects;

};


inline bool Heap::isObjectInNursery(const pVMObject obj) {
	return (int32_t) obj >= (int)nursery && (int32_t) obj < ((int32_t)nursery +
			nurserySize);
}

inline void Heap::writeBarrier(const pVMObject holder, const pVMObject referencedObject) {
#ifdef DEBUG
	writeBarrierCalledOn.insert(make_pair(holder, referencedObject));
#endif

	//we have to add this item to the list if holder is an "old" object and referenced object is a "young" object
	//   we can return if holder is located inside the buffer or referencedObject is not located inside the buffer
	if  (isObjectInNursery(referencedObject) && !isObjectInNursery(holder))
		addToList(holder);
}
#endif
