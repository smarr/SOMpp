#pragma once
#ifndef VMOBJECT_H_
#define VMOBJECT_H_

#include <assert.h>

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
#include <iostream>
#include <cstring>

#include "AbstractObject.h"

#include "../misc/defs.h"
#include "../vm/Universe.h"

#include "ObjectFormats.h"

class VMSymbol;
class VMClass;

//this macro returns a shifted ptr by offset bytes
#define SHIFTED_PTR(ptr, offset) ((void*)((int)(ptr)+(int)(offset)))

/* chbol: this table is not correct anymore because of introduction of
 * class AbstractVMObject
 **************************VMOBJECT****************************
 * __________________________________________________________ *
 *| vtable*          |   0x00 - 0x03                         |*
 *|__________________|_______________________________________|*
 *| hash             |   0x04 - 0x07                         |*
 *| objectSize       |   0x08 - 0x0b                         |*
 *| numberOfFields   |   0x0c - 0x0f                         |*
 *| gcField          |   0x10 - 0x13 (because of alignment)  |*
 *| clazz            |   0x14 - 0x17                         |*
 *|__________________|___0x18________________________________|*
 *                                                            *
 **************************************************************
 */

class VMObject: public AbstractVMObject {

public:
	/* Constructor */
	VMObject(int numberOfFields = 0);

	/* Virtual member functions */
	virtual pVMClass GetClass() const;
	virtual void SetClass(pVMClass cl);
	virtual pVMSymbol GetFieldName(int index) const;
	virtual int GetNumberOfFields() const;
	virtual void SetNumberOfFields(int nof);
	virtual pVMObject GetField(int index) const;
	virtual void Assert(bool value) const;
	virtual void SetField(int index, pVMObject value);
	virtual void WalkObjects(pVMObject (pVMObject));
	virtual pVMObject Clone() const;
	virtual int32_t GetObjectSize() const;
	virtual void SetObjectSize(size_t size);

	/* Operators */

	/**
	 * usage: new( <heap> [, <additional_bytes>] ) VMObject( <constructor params> )
	 * num_bytes parameter is set by the compiler.
	 * parameter additional_bytes (a_b) is used for:
	 *   - fields in VMObject, a_b must be set to (numberOfFields*sizeof(pVMObject))
	 *   - chars in VMString/VMSymbol, a_b must be set to (Stringlength + 1)
	 *   - array size in VMArray; a_b must be set to (size_of_array*sizeof(VMObect*))
	 *   - fields in VMMethod, a_b must be set to (number_of_bc + number_of_csts*sizeof(pVMObject))
	 */
	void* operator new(size_t numBytes, Heap* heap,
			unsigned int additionalBytes = 0, bool outsideNursery = false) {
		void* mem = NULL;
		size_t objSize = numBytes + additionalBytes;
		if (outsideNursery)
			mem = (void*) heap->AllocateMatureObject(objSize);
		else
			mem = (void*) heap->AllocateNurseryObject(objSize);

		((VMObject*) mem)->objectSize = objSize + PAD_BYTES(objSize);
		return mem;
	}

protected:
	int GetAdditionalSpaceConsumption() const;
	//VMObject essentials
	int32_t hash; ///XXX chbol:Hash not needed anymore.. to be deleted later
	int32_t objectSize; //set by the heap at allocation time
	int32_t numberOfFields;

	//pVMObject* FIELDS;
	//Start of fields. All members beyond this point are indexable
	//through FIELDS-macro instead of the member above.
	//So clazz == FIELDS[0]
	pVMClass clazz;
private:
	static const int VMObjectNumberOfFields;
};

#endif
