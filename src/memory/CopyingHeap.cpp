#include "CopyingHeap.h"
#include "CopyingCollector.h"

#if GC_TYPE==COPYING

#include "../vmobjects/AbstractObject.h"
#include "../vm/Universe.h"

CopyingHeap::CopyingHeap(int objectSpaceSize) : Heap(objectSpaceSize) {
	gc = new CopyingCollector(this);
  size_t bufSize = objectSpaceSize;
	currentBuffer = malloc(bufSize);
	oldBuffer = malloc(bufSize);
	memset(currentBuffer, 0x0, bufSize);
	memset(oldBuffer, 0x0, bufSize);
	currentBufferEnd = (void*)((int32_t)currentBuffer + bufSize);
	collectionLimit = (void*)((int32_t)currentBuffer + ((int32_t)(bufSize *
					0.9)));
	nextFreePosition = currentBuffer;
}


void CopyingHeap::switchBuffers() {
	int32_t bufSize = (int32_t)currentBufferEnd - (int32_t)currentBuffer;
	void* tmp = oldBuffer;
	oldBuffer = currentBuffer;
	currentBuffer = tmp;
	currentBufferEnd = (void*)((int32_t)currentBuffer + bufSize);
	nextFreePosition = currentBuffer;
	collectionLimit = (void*)((int32_t)currentBuffer + (int32_t)(0.9 *
				bufSize));
}


AbstractVMObject* CopyingHeap::AllocateObject(size_t size) {
	size_t paddedSize = size + PAD_BYTES(size);
	AbstractVMObject* newObject = (AbstractVMObject*) nextFreePosition;
	nextFreePosition = (void*)((int32_t)nextFreePosition + paddedSize);
	if (nextFreePosition > currentBufferEnd) {
		cout << "Failed to allocate " << size << " Bytes." << endl;
		_UNIVERSE->Quit(-1);
	}
	//let's see if we have to trigger the GC
	if (nextFreePosition > collectionLimit)
		triggerGC();
	return newObject;
}
#endif
