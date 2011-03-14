/*
 * AbstractVMObject.h
 *
 *  Created on: 10.03.2011
 *      Author: christian
 */

#ifndef ABSTRACTOBJECT_H_
#define ABSTRACTOBJECT_H_
#include "../misc/defs.h"
#include "ObjectFormats.h"
#include "../memory/Heap.h"

/*
 * macro for padding - only word-aligned memory must be allocated
 */
#define PAD_BYTES(N) ((sizeof(void*) - ((N) % sizeof(void*))) % sizeof(void*))

class VMClass;
class VMObject;

//this is the base class for all VMObjects
class AbstractVMObject {
protected:
	int32_t gcfield;
public:
	virtual int32_t GetHash();
	virtual pVMClass GetClass() const = 0;
	virtual AbstractVMObject* Clone() const = 0;
	virtual void Send(StdString, AbstractVMObject**, int);
	int32_t GetGCField() const;
	void SetGCField(int32_t);
	virtual int32_t GetObjectSize() const = 0;

	inline virtual void WalkObjects(AbstractVMObject* (AbstractVMObject*)) {
		return;
	}

	void* operator new(size_t numBytes, Heap* heap,
			unsigned int additionalBytes = 0) {
		void* mem = (void*) heap->AllocateObject(numBytes + additionalBytes);
		return mem;
	}
};

#endif /* ABSTRACTOBJECT_H_ */
