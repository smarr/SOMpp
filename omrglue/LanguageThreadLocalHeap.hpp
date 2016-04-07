/*******************************************************************************
 *
 * (c) Copyright IBM Corp. 2014, 2016
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

#ifndef LANGUAGETHREADLOCALHEAP_HPP_
#define LANGUAGETHREADLOCALHEAP_HPP_

#include "omr.h"
#include "LanguageThreadLocalHeapStruct.h"
#include "misc/defs.h"
#include "memory/OMRHeap.h"

#if defined(OMR_GC_THREAD_LOCAL_HEAP)

class MM_LanguageThreadLocalHeap {
private:
protected:
public:
	LanguageThreadLocalHeapStruct* getLanguageThreadLocalHeapStruct(MM_EnvironmentBase* env, bool zeroTLH)
	{
#if defined(OMR_GC_NON_ZERO_TLH)
		if (!zeroTLH) {
			return &((SOM_Thread *)env->getLanguageVMThread())->omrTlh.nonZeroAllocateThreadLocalHeap;
		}
#endif /* defined(OMR_GC_NON_ZERO_TLH) */
		return &((SOM_Thread *)env->getLanguageVMThread())->omrTlh.allocateThreadLocalHeap;
	}

	uint8_t ** getPointerToHeapAlloc(MM_EnvironmentBase* env, bool zeroTLH) {
#if defined(OMR_GC_NON_ZERO_TLH)
		if (!zeroTLH) {
			return &((SOM_Thread *)env->getLanguageVMThread())->omrTlh.nonZeroHeapAlloc;
		}
#endif /* defined(OMR_GC_NON_ZERO_TLH) */
		return &((SOM_Thread *)env->getLanguageVMThread())->omrTlh.heapAlloc;
	}

	uint8_t ** getPointerToHeapTop(MM_EnvironmentBase* env, bool zeroTLH) {
#if defined(OMR_GC_NON_ZERO_TLH)
		if (!zeroTLH) {
			return &((SOM_Thread *)env->getLanguageVMThread())->omrTlh.nonZeroHeapTop;
		}
#endif /* defined(OMR_GC_NON_ZERO_TLH) */
		return &((SOM_Thread *)env->getLanguageVMThread())->omrTlh.heapTop;
	}

	intptr_t * getPointerToTlhPrefetchFTA(MM_EnvironmentBase* env, bool zeroTLH) {
#if defined(OMR_GC_NON_ZERO_TLH)
		if (!zeroTLH) {
			return &((SOM_Thread *)env->getLanguageVMThread())->omrTlh.nonZeroTlhPrefetchFTA;
		}
#endif /* defined(OMR_GC_NON_ZERO_TLH) */
		return &((SOM_Thread *)env->getLanguageVMThread())->omrTlh.tlhPrefetchFTA;
	}

	MM_LanguageThreadLocalHeap() {};

};

#endif /* OMR_GC_THREAD_LOCAL_HEAP */

#endif /* LANGUAGETHREADLOCALHEAP_HPP_ */
