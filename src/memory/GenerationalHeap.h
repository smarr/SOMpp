#pragma once

#include "../misc/defs.h"
#include <assert.h>
#if GC_TYPE ==GENERATIONAL

#include "Heap.h"
#include "../vmobjects/VMObjectBase.h"

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


class GenerationalHeap : public Heap {
  friend class GenerationalCollector;
 public:
	GenerationalHeap(int objectSpaceSize = 1048576);
  AbstractVMObject* AllocateNurseryObject(size_t size);
  AbstractVMObject* AllocateMatureObject(size_t size);
  int32_t GetMaxNurseryObjectSize();
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
 private:
	void* nursery;
	int32_t nursery_end;
	int32_t nurserySize;
	int32_t maxNurseryObjSize;
	int32_t matureObjectsSize;
	void* nextFreePosition;
#ifdef USE_TAGGING
	void writeBarrier_OldHolder(AbstractVMObject* holder, const
			AbstractVMObject* referencedObject);
#else
	void writeBarrier_OldHolder(pVMObject holder, const pVMObject
			referencedObject);
#endif
  void* collectionLimit;
	vector<int>* oldObjsWithRefToYoungObjs;
  vector<pVMObject>* allocatedObjects;
};

#ifdef USE_TAGGING
inline bool GenerationalHeap::isObjectInNursery(const AbstractVMObject* obj) {
#else
inline bool GenerationalHeap::isObjectInNursery(const pVMObject obj) {
#endif
	return (int32_t) obj >= (int)nursery && (int32_t) obj < ((int32_t)nursery +
			nurserySize);
}

inline int32_t GenerationalHeap::GetMaxNurseryObjectSize() {
	return maxNurseryObjSize;
}

#ifdef USE_TAGGING
inline void GenerationalHeap::writeBarrier(AbstractVMObject* holder, const AbstractVMObject* referencedObject) {
#else
inline void GenerationalHeap::writeBarrier(pVMObject holder, const pVMObject referencedObject) {
#endif
#ifdef DEBUG
	//XXX Disabled because of speed reasons --> causes some tests to fail
	//writeBarrierCalledOn.insert(make_pair(holder, referencedObject));
#endif

  int gcfield = *(((int32_t*)holder)+1);
  if ((gcfield & 6) == 2)
		writeBarrier_OldHolder(holder, referencedObject);
}

#endif

