#pragma once

#include "../misc/defs.h"
#include <assert.h>
#if GC_TYPE ==GENERATIONAL

#include "Heap.h"
#include "../vmobjects/VMObjectBase.h"

#ifdef DEBUG
struct VMObjectCompare {
  bool operator() (pair<const pVMObject, const pVMObject> lhs, pair<const
		  pVMObject, const pVMObject> rhs) const
  {return (size_t)lhs.first<(size_t)rhs.first &&
	  (size_t)lhs.second<(size_t)rhs.second ;}
};
#endif


class GenerationalHeap : public Heap {
  friend class GenerationalCollector;
 public:
	GenerationalHeap(long objectSpaceSize = 1048576);
  AbstractVMObject* AllocateNurseryObject(size_t size);
  AbstractVMObject* AllocateMatureObject(size_t size);
  size_t GetMaxNurseryObjectSize();
	void writeBarrier(VMOBJECT_PTR holder, const VMOBJECT_PTR referencedObject);
	inline bool isObjectInNursery(const pVMObject obj);
#ifdef DEBUG
	std::set<pair<const VMOBJECT_PTR, const VMOBJECT_PTR>, VMObjectCompare > writeBarrierCalledOn;
#endif
 private:
	void* nursery;
	size_t nursery_end;
	size_t nurserySize;
	size_t maxNurseryObjSize;
	size_t matureObjectsSize;
	void* nextFreePosition;
	void writeBarrier_OldHolder(VMOBJECT_PTR holder, const VMOBJECT_PTR
			referencedObject);
  void* collectionLimit;
	vector<size_t>* oldObjsWithRefToYoungObjs;
  vector<VMOBJECT_PTR>* allocatedObjects;
};

inline bool GenerationalHeap::isObjectInNursery(const pVMObject obj) {
	return (size_t) obj >= (size_t)nursery && (size_t) obj < nursery_end;
}

inline size_t GenerationalHeap::GetMaxNurseryObjectSize() {
	return maxNurseryObjSize;
}

inline void GenerationalHeap::writeBarrier(VMOBJECT_PTR holder, const VMOBJECT_PTR referencedObject) {
#ifdef DEBUG
	//XXX Disabled because of speed reasons --> causes some tests to fail
	//writeBarrierCalledOn.insert(make_pair(holder, referencedObject));
#endif

  size_t gcfield = *(((size_t*)holder)+1);
  if ((gcfield & 6) == 2)
		writeBarrier_OldHolder(holder, referencedObject);
}

#endif

