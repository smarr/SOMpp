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

#ifndef MARKINGDELEGATE_HPP_
#define MARKINGDELEGATE_HPP_

#include "modronbase.h"
#include "omr.h"

#include "objectdescription.h"
#include "omrgcconsts.h"

#include "CollectorLanguageInterface.hpp"
#include "EnvironmentBase.hpp"
#include "GCExtensionsBase.hpp"
#include "MixedObjectScanner.hpp"
#include "ObjectScannerState.hpp"
#include "Task.hpp"

class MM_CompactScheme;
class MM_EnvironmentStandard;
class MM_ForwardedHeader;
class MM_MarkingScheme;
class MM_MemorySubSpaceSemiSpace;
class MM_ScanClassesMode;

/**
 * Class representing a collector language interface.  This implements the API between the OMR
 * functionality and the language being implemented.
 * @ingroup GC_Base
 */
class MM_MarkingDelegate {
private:
protected:
  //    OMR_VM *_omrVM;
  //	MM_GCExtensionsBase *_extensions;
	MM_MarkingScheme *_markingScheme;
        GC_MixedObjectScanner* _scannerAlloc;
public:
	enum AttachVMThreadReason {
		ATTACH_THREAD = 0x0,
		ATTACH_GC_DISPATCHER_THREAD = 0x1,
		ATTACH_GC_HELPER_THREAD = 0x2,
		ATTACH_GC_MASTER_THREAD = 0x3,
	};

private:
public:
  MMINLINE void workerCleanupAfterGC(MM_EnvironmentBase*) {}
  MMINLINE void masterCleanupAfterGC(MM_EnvironmentBase*) {}

  virtual ~MM_MarkingDelegate();
  
  bool initialize(MM_EnvironmentBase *env, MM_MarkingScheme *markingScheme);
  //void tearDown(OMR_VM *omrVM);

  MM_MarkingDelegate() // OMR_VM *omrVM)
      //    : _omrVM(omrVM)
      //    , _extensions(MM_GCExtensionsBase::getExtensions(omrVM))
  {
    //    _typeId = __FUNCTION__;
  }

  MMINLINE void masterSetupForGC(MM_EnvironmentBase *env) { }

  MMINLINE void masterSetupForWalk(MM_EnvironmentBase *env) { }

  MMINLINE void workerSetupForGC(MM_EnvironmentBase *env) { }

  MMINLINE void workerCompleteGC(MM_EnvironmentBase *env)
  {
    /* All threads flush buffers before this point, and complete any remaining language-specific marking tasks */
    if (env->_currentTask->synchronizeGCThreadsAndReleaseSingleThread(env, UNIQUE_ID)) {
      /* Perform single-threaded tasks here */
      env->_currentTask->releaseSynchronizedGCThreads(env);
    }
  }

  virtual void scanRoots(MM_EnvironmentBase *env);

  virtual GC_ObjectScanner *getObjectScanner(MM_EnvironmentBase *env, omrobjectptr_t objectPtr, GC_ObjectScannerState*, MM_MarkingSchemeScanReason reason, uintptr_t *sizeToDo);

  MMINLINE void completeMarking(MM_EnvironmentBase *env) { }

  MMINLINE void handleWorkPacketOverflowItem(MM_EnvironmentBase *env, omrobjectptr_t objectPtr) { }  
  
  //  MMINLINE void markObject(MM_EnvironmentBase *env, omrobjectptr_t object) {_markingScheme->inlineMarkObject(env, object, false);}
};

#endif /* MARKINGDELEGATE_HPP_ */
