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
#include <cstdlib>
#include "GarbageCollector.h"
#include "../misc/defs.h"
#include "../vmobjects/ObjectFormats.h"
#ifdef USE_TAGGING
#include "../vmobjects/VMPointer.h"
#endif

class AbstractVMObject;
using namespace std;
//macro to access the heap
#define _HEAP Heap::GetHeap()
#if GC_TYPE==GENERATIONAL
#ifdef DEBUG
struct VMObjectCompare {
#ifdef USE_TAGGING
  bool operator() (pair<const AbstractVMObject*, const AbstractVMObject*> lhs, pair<const
		  AbstractVMObject*, const AbstractVMObject*> rhs) const
#else
  bool operator() (pair<const pVMObject, const pVMObject> lhs, pair<const
		  pVMObject, const pVMObject> rhs) const
#endif
  {return (int32_t)lhs.first<(int32_t)rhs.first &&
	  (int32_t)lhs.second<(int32_t)rhs.second ;}
};
#endif
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
#if GC_TYPE==GENERATIONAL
    AbstractVMObject* AllocateNurseryObject(size_t size);
	AbstractVMObject* AllocateMatureObject(size_t size);
	int32_t GetMaxNurseryObjectSize();
	inline void FreeObject(AbstractVMObject* obj);
#else
    AbstractVMObject* AllocateObject(size_t size);
#endif
	inline void triggerGC(void);
	bool isCollectionTriggered(void);
    void FullGC();
#if GC_TYPE==GENERATIONAL
#ifdef USE_TAGGING
	void writeBarrier(AbstractVMObject* holder, const AbstractVMObject* referencedObject);
	inline bool isObjectInNursery(const AbstractVMObject* obj);
#else
	void writeBarrier(pVMObject holder, const pVMObject referencedObject);
	inline bool isObjectInNursery(const pVMObject obj);
#endif
#ifdef DEBUG
#ifdef USE_TAGGING
	std::set<pair<const AbstractVMObject*, const AbstractVMObject*>, VMObjectCompare > writeBarrierCalledOn;
#else
	std::set<pair<const pVMObject, const pVMObject>, VMObjectCompare > writeBarrierCalledOn;
#endif
#endif
#else
	inline void FreeObject(AbstractVMObject* o);
#endif
private:
    static Heap* theHeap;
#if GC_TYPE==GENERATIONAL
#ifdef USE_TAGGING
	void writeBarrier_OldHolder(AbstractVMObject* holder, const
			AbstractVMObject* referencedObject);
#else
	void writeBarrier_OldHolder(pVMObject holder, const pVMObject
			referencedObject);
#endif
#endif

	//members for moving GC
#if GC_TYPE==GENERATIONAL
	void* nursery;
	int32_t nursery_end;
	int32_t nurserySize;
	int32_t maxNurseryObjSize;
	int32_t matureObjectsSize;
#else
	void* currentBuffer;
	void* oldBuffer;
	void* currentBufferEnd;
	void switchBuffers(void);
#endif
	void* nextFreePosition;

	//flag that shows if a Collection is triggered
	bool gcTriggered;
	GarbageCollector* gc;
    void* collectionLimit;
#if GC_TYPE==GENERATIONAL
	vector<int>* oldObjsWithRefToYoungObjs;
	vector<pVMObject>* allocatedObjects;
#endif

};

#if GC_TYPE==GENERATIONAL
#ifdef USE_TAGGING
inline bool Heap::isObjectInNursery(const AbstractVMObject* obj) {
#else

inline bool Heap::isObjectInNursery(const pVMObject obj) {
#endif
	return (int32_t) obj >= (int)nursery && (int32_t) obj < ((int32_t)nursery +
			nurserySize);
}

inline int32_t Heap::GetMaxNurseryObjectSize() {
	return maxNurseryObjSize;
}
#ifdef USE_TAGGING
inline void Heap::writeBarrier(AbstractVMObject* holder, const AbstractVMObject* referencedObject) {
#else

inline void Heap::writeBarrier(pVMObject holder, const pVMObject referencedObject) {
#endif
#ifdef DEBUG
	//XXX Disabled because of speed reasons --> causes some tests to fail
	//writeBarrierCalledOn.insert(make_pair(holder, referencedObject));
#endif

	if ((int32_t)holder < (int32_t)nursery || (int32_t)holder > nursery_end)
		writeBarrier_OldHolder(holder, referencedObject);
}
#endif
#endif

void Heap::triggerGC(void) {
	gcTriggered = true;
}

void Heap::FreeObject(AbstractVMObject* o) {
	free(o);
}

