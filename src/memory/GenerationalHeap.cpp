#include "GenerationalHeap.h"
#include "GenerationalCollector.h"
#include "../vmobjects/AbstractObject.h"
#include "../vm/Universe.h"

#include <string.h>
#include <iostream>

#if GC_TYPE == GENERATIONAL

using namespace std;

GenerationalHeap::GenerationalHeap(int objectSpaceSize) {
	//our initial collection limit is 90% of objectSpaceSize
	//collectionLimit = objectSpaceSize * 0.9;
	gc = new GenerationalCollector(this);

	nursery = malloc(objectSpaceSize);
	nurserySize = objectSpaceSize;
	maxNurseryObjSize = objectSpaceSize / 2;
	nursery_end = (int32_t)nursery + nurserySize;
	matureObjectsSize = 0;
	memset(nursery, 0x0, objectSpaceSize);
	collectionLimit = (void*)((int32_t)nursery + ((int32_t)(objectSpaceSize *
					0.9)));
	nextFreePosition = nursery;
	allocatedObjects = new vector<pVMObject>();
	oldObjsWithRefToYoungObjs = new vector<int>();
}

#if GC_TYPE==GENERATIONAL
AbstractVMObject* GenerationalHeap::AllocateNurseryObject(size_t size) {
	size_t paddedSize = size + PAD_BYTES(size);
	AbstractVMObject* newObject = (AbstractVMObject*) nextFreePosition;
	nextFreePosition = (void*)((int32_t)nextFreePosition + (int32_t)paddedSize);
	if ((int32_t)nextFreePosition > (int32_t)nursery + nurserySize) {
		cout << "Failed to allocate " << size << " Bytes in nursery." << endl;
		_UNIVERSE->Quit(-1);
	}
	//let's see if we have to trigger the GC
	if (nextFreePosition > collectionLimit)
		triggerGC();
	return newObject;
}

AbstractVMObject* GenerationalHeap::AllocateMatureObject(size_t size) {
	size_t paddedSize = size + PAD_BYTES(size);
#ifdef USE_TAGGING
	AbstractVMObject* newObject = (AbstractVMObject*)malloc(paddedSize);
#else
	pVMObject newObject = (pVMObject)malloc(paddedSize);
#endif
	if (newObject == NULL) {
		cout << "Failed to allocate " << size << " Bytes." << endl;
		_UNIVERSE->Quit(-1);
	}
	allocatedObjects->push_back(newObject);
	matureObjectsSize += paddedSize;
	return newObject;
}
#endif

#ifdef USE_TAGGING
void GenerationalHeap::writeBarrier_OldHolder(AbstractVMObject* holder, const AbstractVMObject*
#else
void GenerationalHeap::writeBarrier_OldHolder(pVMObject holder, const pVMObject
#endif
		referencedObject) {
	if (isObjectInNursery(referencedObject)
			&& ((holder->GetGCField() & MASK_SEEN_BY_WRITE_BARRIER) ==false)) {
		oldObjsWithRefToYoungObjs->push_back((int32_t)holder);
		holder->SetGCField(holder->GetGCField() | MASK_SEEN_BY_WRITE_BARRIER);
	}
}

#endif
