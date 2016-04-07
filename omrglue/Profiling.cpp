/*******************************************************************************
 *
 * (c) Copyright IBM Corp. 2016
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

#include "omr.h"
#include "omrprofiler.h"
#include "Profiling.h"

#include "vm/Universe.h"
#include "vmobjects/VMFrame.h"
#include "vmobjects/VMMethod.h"
#include "vmobjects/VMSymbol.h"

#define SOM_OMR_SAMPLESTACK_BACKOFF_MAX 50
#define SOM_OMR_SAMPLESTACK_BACKOFF_TIMER_DECR 1

#define SOM_OMR_PROF_METHOD_NAME_IDX 0
#define SOM_OMR_PROF_CLASS_NAME_IDX 1

static void som_omr_sampleStack(OMR_VMThread *omrVMThread, VMFrame *frame);
static void som_omr_insertMethodEntryInMethodDictionary(OMR_VM *omrVM, VMMethod *method);


extern "C" {

#define SOM_OMR_METHOD_PROPERTY_COUNT 2

static const char *methodPropertyNames[SOM_OMR_METHOD_PROPERTY_COUNT] = {
	"methodName",
	"className",
};

typedef struct SOM_OMR_MethodDictionaryEntry {
	const void *key;
	const char *propertyValues[SOM_OMR_METHOD_PROPERTY_COUNT];
} SOM_OMR_MethodDictionaryEntry;

/**
 * Get the number of method properties
 * @return Number of method properties
 */
int
OMR_Glue_GetMethodDictionaryPropertyNum(void)
{
	return SOM_OMR_METHOD_PROPERTY_COUNT;
}

/**
 * Get the method property names
 * @return Method property names
 */
const char * const *
OMR_Glue_GetMethodDictionaryPropertyNames(void)
{
	return methodPropertyNames;
}
}

static void
som_omr_sampleStack(OMR_VMThread *omrVMThread, VMFrame *frame)
{
	unsigned int frameDepth = 0;

	while ((NULL != frame) && (frameDepth < 10)) {
		VMMethod *method = frame->GetMethod();

		if (0 == frameDepth) {
			omr_ras_sampleStackTraceStart(omrVMThread, method);
		} else {
			omr_ras_sampleStackTraceContinue(omrVMThread, method);
		}

		frameDepth += 1;
		if (frame->HasPreviousFrame()) {
			frame = frame->GetPreviousFrame();
		} else {
			frame = NULL;
		}
	}
}

void
som_omr_checkSampleStack(VMFrame *frame)
{
	OMR_VMThread *omrVMThread = omr_vmthread_getCurrent(GetHeap<OMRHeap>()->getOMRVM());

	if (0 == omrVMThread->_sampleStackBackoff) {
		omrVMThread->_sampleStackBackoff = SOM_OMR_SAMPLESTACK_BACKOFF_MAX;
		if (omr_ras_sampleStackEnabled()) {
			som_omr_sampleStack(omrVMThread, frame);
		}
	}
	if (SOM_OMR_SAMPLESTACK_BACKOFF_TIMER_DECR > omrVMThread->_sampleStackBackoff) {
		omrVMThread->_sampleStackBackoff = 0;
	} else {
		omrVMThread->_sampleStackBackoff -= SOM_OMR_SAMPLESTACK_BACKOFF_TIMER_DECR;
	}
}

static void
som_omr_insertMethodEntryInMethodDictionary(OMR_VM *omrVM, VMMethod *method)
{
	omr_error_t rc = OMR_ERROR_NONE;
	if ((NULL != omrVM->_methodDictionary) && (NULL != method)) {
		SOM_OMR_MethodDictionaryEntry tempEntry;
		VMSymbol *methodSym = method->GetSignature();
		VMClass *holder = method->GetHolder();

		memset(&tempEntry, 0, sizeof(tempEntry));
		tempEntry.key = method;
		tempEntry.propertyValues[SOM_OMR_PROF_METHOD_NAME_IDX] = methodSym->GetChars();
		if (NULL == holder) {
			/* This should never happen for a properly initialized method in a properly initialized class. */
			tempEntry.propertyValues[SOM_OMR_PROF_CLASS_NAME_IDX] = "<unkown>";
		} else {
			VMSymbol *holderSym = holder->GetName();
			tempEntry.propertyValues[SOM_OMR_PROF_CLASS_NAME_IDX] = holderSym->GetChars();
		}

		rc = omr_ras_insertMethodDictionary(omrVM, (OMR_MethodDictionaryEntry *)&tempEntry);
		if (OMR_ERROR_NONE != rc) {
			Universe::ErrorPrint("som_omr_insertMethodEntryInMethodDictionary failed\n");
		}
	}
}

void
som_omr_add_class_methods_to_dictionary(VMClass *clazz)
{
	long i = 0;
	OMR_VM *omrVM = GetHeap<OMRHeap>()->getOMRVM();
	for (i = 0; i < clazz->GetNumberOfInstanceInvokables(); i += 1) {
		VMMethod *method = (VMMethod *)clazz->GetInstanceInvokable(i);
		som_omr_insertMethodEntryInMethodDictionary(omrVM, method);
	}
}
