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

#include "ModronAssertions.h"
#include "ObjectAllocationModel.hpp"
#include "ObjectModelBase.hpp"
#include "ObjectModel.hpp"

omrobjectptr_t
GC_ObjectModel::initializeAllocation(MM_EnvironmentBase *env, void *allocatedBytes, MM_AllocateInitialization *allocateInitialization)
{
	Assert_MM_true(MM_ObjectAllocationModel::allocation_category_example == allocateInitialization->getAllocationCategory());

	omrobjectptr_t objectPtr = NULL;
	if (NULL != allocatedBytes) {
		MM_ObjectAllocationModel *objectAllocationModel = (MM_ObjectAllocationModel *)allocateInitialization;
		objectPtr = objectAllocationModel->initializeObject(env, allocatedBytes);
	}
	return objectPtr;
}

/**
 * Set flags in object header. The bitsToClear and bitsToSet masks are expected to be unshifted
 * (aligned in low-order byte).
 *
 * @param objectPtr Pointer to an object
 * @param bitsToClear unshifted mask to clear bits
 * @param bitsToSet unshifted mask to set bits
 */
void
GC_ObjectModel::setObjectFlags(omrobjectptr_t objectPtr, uintptr_t bitsToClear, uintptr_t bitsToSet)
{
#if defined(OBJECT_MODEL_MODRON_ASSERTIONS)
  Assert_MM_true(0 == (~(fomrobject_t)OMR_OBJECT_METADATA_FLAGS_MASK & (fomrobject_t)bitsToClear));
  Assert_MM_true(0 == (~(fomrobject_t)OMR_OBJECT_METADATA_FLAGS_MASK & (fomrobject_t)bitsToSet));
#endif /* defined(OBJECT_MODEL_MODRON_ASSERTIONS) */

  fomrobject_t* flagsPtr = getObjectHeaderSlotAddress(objectPtr);
  fomrobject_t clear = (fomrobject_t)bitsToClear << getObjectHeaderSlotFlagsShift();
  fomrobject_t set = (fomrobject_t)bitsToSet << getObjectHeaderSlotFlagsShift();

  *flagsPtr = (*flagsPtr & ~clear) | set;
}

fomrobject_t*
GC_ObjectModel::getObjectHeaderSlotAddress(omrobjectptr_t objectPtr)
{
  return (fomrobject_t*)(objectPtr);// + _delegate.getObjectHeaderSlotOffset();
}

/**
 * Get the bit offset to the flags byte in object headers.
 */
uintptr_t
GC_ObjectModel::getObjectHeaderSlotFlagsShift()
{
  return 0;//_delegate.getObjectHeaderSlotFlagsShift();
}

/**
 * Get the value of the flags byte from the header of an object. The flags value is
 * returned in the low-order byte of the returned value.
 *
 * @param objectPtr the object to extract the flags byte from
 * @return the value of the flags byte from the object header
 */
uintptr_t
GC_ObjectModel::getObjectFlags(omrobjectptr_t objectPtr)
{
  return ((*getObjectHeaderSlotAddress(objectPtr)) >> getObjectHeaderSlotFlagsShift()) & (fomrobject_t)OMR_OBJECT_METADATA_FLAGS_MASK;
}
  
