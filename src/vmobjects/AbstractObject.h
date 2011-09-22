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
#include "VMObjectBase.h"
/*
 * macro for padding - only word-aligned memory must be allocated
 */
#define PAD_BYTES(N) ((sizeof(void*) - ((N) % sizeof(void*))) % sizeof(void*))

class VMClass;
class VMObject;
class VMSymbol;

#include <iostream>
#include <assert.h>
using namespace std;

//this is the base class for all VMObjects
class AbstractVMObject : public VMObjectBase {
public:
	virtual int32_t GetHash();
	virtual pVMClass GetClass() const = 0;
#ifdef USE_TAGGING
	virtual AbstractVMObject* Clone() const = 0;
#else
	virtual pVMObject Clone() const = 0;
#endif
	virtual void Send(StdString, pVMObject*, int);
	virtual int32_t GetObjectSize() const = 0;
	AbstractVMObject() {
		gcfield = 0;
	}
	inline virtual void SetObjectSize(size_t size) {
		cout << "this object doesn't support SetObjectSize" << endl;
		throw "this object doesn't support SetObjectSize";
	}
	inline virtual int GetNumberOfFields() const {
		cout << "this object doesn't support GetNumberOfFields" << endl;
		throw "this object doesn't support GetNumberOfFields";
	}
	virtual void SetNumberOfFields(int nof) {
		cout << "this object doesn't support SetNumberOfFields" << endl;
		throw "this object doesn't support SetNumberOfFields";
	}
	inline virtual void SetClass(pVMClass cl) {
		cout << "this object doesn't support SetClass" << endl;
		throw "this object doesn't support SetClass";
	}
	int GetFieldIndex(pVMSymbol fieldName) const;
	inline virtual void SetField(int index, pVMObject value) {
		cout << "this object doesn't support SetField" << endl;
		throw "this object doesn't support SetField";
	}
	virtual pVMObject GetField(int index) const;
#ifdef USE_TAGGING
	inline virtual void WalkObjects(AbstractVMObject* (AbstractVMObject*)) {
#else
	inline virtual void WalkObjects(pVMObject (pVMObject)) {
#endif
		return;
	}
	inline virtual pVMSymbol GetFieldName(int index) const {
		cout << "this object doesn't support GetFieldName" << endl;
		throw "this object doesn't support GetFieldName";
	}

	void* operator new(size_t numBytes, Heap* heap,
			unsigned int additionalBytes = 0, bool outsideNursery = false) {
		//if outsideNursery flag is set or object is too big for nursery, we
		// allocate a mature object
		if (outsideNursery)
			return (void*) heap->AllocateMatureObject(numBytes +
					additionalBytes);
		assert(numBytes + additionalBytes < _HEAP->GetMaxNurseryObjectSize());
		return (void*) heap->AllocateNurseryObject(numBytes + additionalBytes);
	}
};

#endif /* ABSTRACTOBJECT_H_ */
