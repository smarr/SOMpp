/*******************************************************************************
 *
 * (c) Copyright IBM Corp. 1991, 2016
 *
 *  This program and the accompanying materials are made available
 *  under the terms of the Eclipse Public License v1.0 and
 *  Apache License v2.0 which accompanies this distribution.
 *
 *      The Eclipse Public License is available at
 *      http://www.eclipse.org/legal/epl-v10.html
 *
 *      The Apache License v2.0 is available at
 *      http://www.opensource.org/licenses/apache2.0.php
 *
 * Contributors:
 *    Multiple authors (IBM Corp.) - initial implementation and documentation
 *******************************************************************************/

#if !defined(OBJECTMODEL_HPP_)
#define OBJECTMODEL_HPP_

#include "omrgc.h"

#include "ModronAssertions.h"
#include "modronbase.h"
#include "objectdescription.h"
#include "Bits.hpp"
#include "HeapLinkedFreeHeader.hpp"

#include "misc/defs.h"
#include "vmobjects/AbstractObject.h"
#include "vmobjects/ObjectFormats.h"

class MM_GCExtensionsBase;

#define J9_GC_OBJECT_ALIGNMENT_IN_BYTES 0x8
#define J9_GC_MINIMUM_OBJECT_SIZE 0x10

/**
 * Provides information for a given object.
 * @ingroup GC_Base
 */
class GC_ObjectModel
{
/*
 * Member data and types
 */
private:
	uintptr_t _objectAlignmentInBytes; /**< Cached copy of object alignment for getting object alignment for adjusting for alignment */
	uintptr_t _objectAlignmentShift; /**< Cached copy of object alignment shift, must be log2(_objectAlignmentInBytes)  */

protected:
public:

/*
 * Member functions
 */
private:
protected:
public:
	/**
	 * Initialize the receiver, a new instance of GC_ObjectModel
	 *
	 * @return true on success, false on failure
	 */
	bool
	initialize(MM_GCExtensionsBase *extensions)
	{
	  _objectAlignmentInBytes = 0x8;
	  _objectAlignmentShift = 3;
	  return true;
	}

	void tearDown(MM_GCExtensionsBase *extensions) {}

        omrobjectptr_t
	initializeAllocation(MM_EnvironmentBase *, void*, MM_AllocateInitialization*);

  uintptr_t
  getObjectFlags(omrobjectptr_t objectPtr);
  
	MMINLINE uintptr_t
	adjustSizeInBytes(uintptr_t sizeInBytes)
	{
		sizeInBytes =  (sizeInBytes + (_objectAlignmentInBytes - 1)) & (uintptr_t)~(_objectAlignmentInBytes - 1);

#if defined(OMR_GC_MINIMUM_OBJECT_SIZE)
		if (sizeInBytes < J9_GC_MINIMUM_OBJECT_SIZE) {
			sizeInBytes = J9_GC_MINIMUM_OBJECT_SIZE;
		}
#endif /* OMR_GC_MINIMUM_OBJECT_SIZE */

		return sizeInBytes;
	}

	/**
	 * Returns TRUE if an object is dead, FALSE otherwise.
	 * @param objectPtr Pointer to an object
	 * @return TRUE if an object is dead, FALSE otherwise
	 */
	MMINLINE bool
	isDeadObject(void *objectPtr)
	{
		return 0 != (*((uintptr_t *)objectPtr) & J9_GC_OBJ_HEAP_HOLE_MASK);
	}

	/**
	 * Returns TRUE if an object is a dead single slot object, FALSE otherwise.
	 * @param objectPtr Pointer to an object
	 * @return TRUE if an object is a dead single slot object, FALSE otherwise
	 */
	MMINLINE bool
	isSingleSlotDeadObject(omrobjectptr_t objectPtr)
	{
		return J9_GC_SINGLE_SLOT_HOLE == (*((uintptr_t *)objectPtr) & J9_GC_OBJ_HEAP_HOLE_MASK);
	}

	/**
	 * Returns the size, in bytes, of a single slot dead object.
	 * @param objectPtr Pointer to an object
	 * @return The size, in bytes, of a single slot dead object
	 */
	MMINLINE uintptr_t
	getSizeInBytesSingleSlotDeadObject(omrobjectptr_t objectPtr)
	{
		return sizeof(uintptr_t);
	}

	/**
	 * Returns the size, in bytes, of a multi-slot dead object.
	 * @param objectPtr Pointer to an object
	 * @return The size, in bytes, of a multi-slot dead object
	 */
	MMINLINE uintptr_t getSizeInBytesMultiSlotDeadObject(omrobjectptr_t objectPtr)
	{
		return MM_HeapLinkedFreeHeader::getHeapLinkedFreeHeader(objectPtr)->getSize();
	}

	/**
	 * Returns the size in bytes of a dead object.
	 * @param objectPtr Pointer to an object
	 * @return The size in byts of a dead object
	 */
	MMINLINE uintptr_t
	getSizeInBytesDeadObject(omrobjectptr_t objectPtr)
	{
		if(isSingleSlotDeadObject(objectPtr)) {
			return getSizeInBytesSingleSlotDeadObject(objectPtr);
		}
		return getSizeInBytesMultiSlotDeadObject(objectPtr);
	}

	MMINLINE uintptr_t
	getConsumedSizeInSlotsWithHeader(omrobjectptr_t objectPtr)
	{
		return MM_Bits::convertBytesToSlots(getConsumedSizeInBytesWithHeader(objectPtr));
	}

	MMINLINE uintptr_t
	getConsumedSizeInBytesWithHeader(omrobjectptr_t objectPtr)
	{
		return adjustSizeInBytes(getSizeInBytesWithHeader(objectPtr));
	}

	MMINLINE uintptr_t
	getConsumedSizeInBytesWithHeaderForMove(omrobjectptr_t objectPtr)
	{
		return getConsumedSizeInBytesWithHeader(objectPtr);
	}

	MMINLINE uintptr_t
	getSizeInBytesWithHeader(omrobjectptr_t objectPtr)
	{
		if (IS_TAGGED(objectPtr)) {
			return 0;
		} else {
			AbstractVMObject* o = (AbstractVMObject*)(objectPtr);
			return o->GetObjectSize();
		}
	}

#if defined(OMR_GC_MODRON_COMPACTION)
	/**
	 * Before objects are moved during compaction is there any language specific updates
	 * @param vmThread - the thread performing the work
	 * @param objectPtr Pointer to an object
	 */
	MMINLINE void
	preMove(OMR_VMThread* vmThread, omrobjectptr_t objectPtr)
	{

	}

	/**
	 * After objects are moved during compaction is there any language specific updates
	 * @param vmThread - the thread performing the work
	 * @param objectPtr Pointer to an object
	 */
	MMINLINE void
	postMove(OMR_VMThread* vmThread, omrobjectptr_t objectPtr)
	{
		/* do nothing */
	}
#endif /* OMR_GC_MODRON_COMPACTION */

#if defined(OMR_GC_MODRON_SCAVENGER)
	/**
	 * Returns TRUE if an object is remembered, FALSE otherwise.
	 * @param objectPtr Pointer to an object
	 * @return TRUE if an object is remembered, FALSE otherwise
	 */
	MMINLINE bool
	isRemembered(omrobjectptr_t objectPtr)
	{
		return false;
	}
#endif /* OMR_GC_MODRON_SCAVENGER */

 	/**
	 * Set run-time Object Alignment in the heap value
	 * Function exists because we can only determine it is way after ObjectModel is init
	 */
	MMINLINE void
	setObjectAlignmentInBytes(uintptr_t objectAlignmentInBytes)
	{
		_objectAlignmentInBytes = objectAlignmentInBytes;
	}

	MMINLINE void
	setObjectAlignment(OMR_VM *omrVM)
	{
//		_objectAlignmentInBytes = OMR_MAX((uintptr_t)1 << omrVM->_compressedPointersShift, OMR_MINIMUM_OBJECT_ALIGNMENT);
//		_objectAlignmentShift = OMR_MAX(omrVM->_compressedPointersShift, OMR_MINIMUM_OBJECT_ALIGNMENT_SHIFT);

		omrVM->_objectAlignmentInBytes = _objectAlignmentInBytes;
		omrVM->_objectAlignmentShift = _objectAlignmentShift;
	}  

 	/**
	 * Set run-time Object Alignment Shift value
	 * Function exists because we can only determine it is way after ObjectModel is init
	 */
	MMINLINE void
	setObjectAlignmentShift(uintptr_t objectAlignmentShift)
	{
		_objectAlignmentShift = objectAlignmentShift;
	}

 	/**
	 * Get run-time Object Alignment in the heap value
	 */
	MMINLINE uintptr_t
	getObjectAlignmentInBytes()
	{
		return _objectAlignmentInBytes;
	}

 	/**
	 * Get run-time Object Alignment Shift value
	 */
	MMINLINE uintptr_t
	getObjectAlignmentShift()
	{
		return _objectAlignmentShift;
	}

        fomrobject_t*
	getObjectHeaderSlotAddress(omrobjectptr_t objectPtr);

        uintptr_t
	getObjectHeaderSlotFlagsShift();

  /**
   * If the received object holds an indirect reference (ie a reference to an object
   * that is not reachable from the object reference graph) a pointer to the referenced
   * object should be returned here. This method is called during heap walks for each
   * heap object.
   *
   * @param objectPtr the object to botain indirct reference from
   * @return a pointer to the indirect object, or NULL if none
   */
  MMINLINE omrobjectptr_t
  getIndirectObject(omrobjectptr_t objectPtr)
  {
    return objectPtr;//_delegate.getIndirectObject(objectPtr);
  }
  
        void
	setObjectFlags(omrobjectptr_t objectPtr, uintptr_t bitsToClear, uintptr_t bitsToSet);
};


#endif /* OBJECTMODEL_HPP_ */

