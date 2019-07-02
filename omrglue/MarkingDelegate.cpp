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

#include "modronbase.h"

#include "CollectorLanguageInterfaceImpl.hpp"
#if defined(OMR_GC_MODRON_CONCURRENT_MARK)
#include "ConcurrentSafepointCallback.hpp"
#endif /* OMR_GC_MODRON_CONCURRENT_MARK */
#if defined(OMR_GC_MODRON_COMPACTION)
#include "CompactScheme.hpp"
#endif /* OMR_GC_MODRON_COMPACTION */
#include "EnvironmentStandard.hpp"
#include "GCExtensionsBase.hpp"
#include "MarkingScheme.hpp"
#include "ObjectIterator.hpp"
#include "mminitcore.h"
#include "omr.h"
#include "omrvm.h"
#include "OMRVMInterface.hpp"
#include "ParallelGlobalGC.hpp"
#include "ParallelTask.hpp"
// #include "ScanClassesMode.hpp"

//#include "Heap.hpp"
#include "HeapRegionIterator.hpp"
#include "ObjectHeapIteratorAddressOrderedList.hpp"

#include "ForwardedHeader.hpp"
#include "GlobalCollector.hpp"
#include "MarkingDelegate.hpp"

//#include "OMRHeap.h"
#include "Universe.h"

bool
MM_MarkingDelegate::initialize(MM_EnvironmentBase *env, MM_MarkingScheme *markingScheme)
{
  //  _objectModel = &(env->getExtensions()->objectModel);
  _scannerAlloc = nullptr;
  _markingScheme = markingScheme;
  
  return true;
}

MM_MarkingDelegate::~MM_MarkingDelegate() {
  if(_scannerAlloc) {
    _scannerAlloc->~GC_MixedObjectScanner();
    MM_EnvironmentBase *env = MM_EnvironmentBase::getEnvironment(omr_vmthread_getCurrent(GetHeap<OMRHeap>()->getOMRVM()));
    env->getForge()->free(_scannerAlloc);
  }
}

GC_ObjectScanner *
MM_MarkingDelegate::getObjectScanner(MM_EnvironmentBase *env, omrobjectptr_t objectPtr, GC_ObjectScannerState*, MM_MarkingSchemeScanReason reason, uintptr_t *sizeToDo)
{
  if(_scannerAlloc) {
    _scannerAlloc->~GC_MixedObjectScanner();
    env->getForge()->free(_scannerAlloc);
  }

  _scannerAlloc = (GC_MixedObjectScanner *)env->getForge()->allocate(sizeof(GC_MixedObjectScanner), MM_AllocationCategory::FIXED, OMR_GET_CALLSITE());
  return GC_MixedObjectScanner::newInstance(env, objectPtr, _scannerAlloc, 1);
}

static gc_oop_t
mark_object(gc_oop_t oop)
{
 if (IS_TAGGED(oop)) {
   return oop;
 }
 
 MM_EnvironmentBase *env = MM_EnvironmentBase::getEnvironment(omr_vmthread_getCurrent(GetHeap<OMRHeap>()->getOMRVM()));
 MM_GCExtensionsBase* extensions = env->getExtensions();
 MM_ParallelGlobalGC* collector = (MM_ParallelGlobalGC* )extensions->getGlobalCollector();
 
 collector->getMarkingScheme()->markObject(env, (omrobjectptr_t)oop);
 return oop;
}

void
MM_MarkingDelegate::scanRoots(MM_EnvironmentBase *env)
{
  if (env->_currentTask->synchronizeGCThreadsAndReleaseSingleThread(env, UNIQUE_ID)) {
    // This walks the globals of the universe, and the interpreter
    GetUniverse()->WalkGlobals(mark_object);
    env->_currentTask->releaseSynchronizedGCThreads(env);
  }
}
