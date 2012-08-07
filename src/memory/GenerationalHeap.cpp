#include "GenerationalHeap.h"
#include "GenerationalCollector.h"
#include "../vmobjects/AbstractObject.h"
#include "../vm/Universe.h"

#include <string.h>
#include <iostream>

#if GC_TYPE == GENERATIONAL

using namespace std;

GenerationalHeap::GenerationalHeap(long objectSpaceSize) {
	//our initial collection limit is 90% of objectSpaceSize
	//collectionLimit = objectSpaceSize * 0.9;
	gc = new GenerationalCollector(this);

	nursery = malloc(objectSpaceSize);
	nurserySize = objectSpaceSize;
	maxNurseryObjSize = objectSpaceSize / 2;
	nursery_end = (size_t)nursery + nurserySize;
	matureObjectsSize = 0;
	memset(nursery, 0x0, objectSpaceSize);
	collectionLimit = (void*)((size_t)nursery + ((size_t)(objectSpaceSize *
					0.9)));
	nextFreePosition = nursery;
	allocatedObjects = new vector<VMOBJECT_PTR>();
	oldObjsWithRefToYoungObjs = new vector<size_t>();
}


AbstractVMObject* GenerationalHeap::AllocateNurseryObject(size_t size) {
  size_t paddedSize = size + PAD_BYTES(size);
  AbstractVMObject* newObject = (AbstractVMObject*) nextFreePosition;
  nextFreePosition = (void*)((size_t)nextFreePosition + (size_t)paddedSize);
  if ((size_t)nextFreePosition > (size_t)nursery + nurserySize) {
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
	VMOBJECT_PTR newObject = (VMOBJECT_PTR)malloc(paddedSize);
	if (newObject == NULL) {
		cout << "Failed to allocate " << size << " Bytes." << endl;
		_UNIVERSE->Quit(-1);
	}
	allocatedObjects->push_back(newObject);
	matureObjectsSize += paddedSize;
	return newObject;
}


void GenerationalHeap::writeBarrier_OldHolder(VMOBJECT_PTR holder, const VMOBJECT_PTR
                                              referencedObject) {
  if (isObjectInNursery(referencedObject)) {
    oldObjsWithRefToYoungObjs->push_back((size_t)holder);
    holder->SetGCField(holder->GetGCField() | MASK_SEEN_BY_WRITE_BARRIER);
  }
}

#endif
