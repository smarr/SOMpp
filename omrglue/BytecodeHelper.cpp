/*******************************************************************************
 *
 * (c) Copyright IBM Corp. 2016, 2016
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

#include "BytecodeHelper.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#include "vmobjects/Signature.h"
#include "vmobjects/VMFrame.h"
#include "vmobjects/VMMethod.h"
#include "vmobjects/VMSymbol.h"
#include "interpreter/bytecodes.h"
#include "interpreter/Interpreter.h"

int64_t
BytecodeHelper::getClass(int64_t object)
{
#define VALUE_FOR_GET_CLASS_LINE LINETOSTR(__LINE__)

	return (int64_t)CLASS_OF(object);
}

int64_t
BytecodeHelper::getSuperClass(int64_t object)
{
#define VALUE_FOR_GET_SUPER_CLASS_LINE LINETOSTR(__LINE__)

	VMClass *clazz = CLASS_OF(object);
	return (int64_t)clazz->GetSuperClass();
}

int64_t
BytecodeHelper::getGlobal(int64_t symbol)
{
#define VALUE_FOR_GET_GLOBAL_LINE LINETOSTR(__LINE__)

	return (int64_t)GetUniverse()->GetGlobal((VMSymbol*)symbol);
}

int64_t
BytecodeHelper::getNewBlock(int64_t framePtr, int64_t blockMethod, int64_t numOfArgs)
{
#define VALUE_FOR_GET_NEW_BLOCK_LINE LINETOSTR(__LINE__)

	VMFrame *frame = (VMFrame *)framePtr;
	return (int64_t)GetUniverse()->NewBlock((VMMethod*)blockMethod, frame, (long)numOfArgs);
}

int64_t
BytecodeHelper::newInteger(int64_t value)
{
#define VALUE_FOR_NEW_INTEGER_LINE LINETOSTR(__LINE__)

	return (int64_t)GetUniverse()->NewInteger(value);
}

int64_t
BytecodeHelper::newDouble(double value)
{
#define VALUE_FOR_NEW_DOUBLE_LINE LINETOSTR(__LINE__)

	return (int64_t)GetUniverse()->NewDouble(value);
}

int64_t
BytecodeHelper::getFieldFrom(int64_t selfPtr, int64_t fieldIndex)
{
#define VALUE_FOR_GET_FIELD_FROM_LINE LINETOSTR(__LINE__)

	vm_oop_t self = (vm_oop_t)selfPtr;
	return (int64_t)((VMObject*)self)->GetField(fieldIndex);
}

void
BytecodeHelper::setFieldTo(int64_t selfPtr, int64_t fieldIndex, int64_t valuePtr)
{
#define VALUE_FOR_SET_FIELD_TO_LINE LINETOSTR(__LINE__)

	vm_oop_t self = (vm_oop_t)selfPtr;
	vm_oop_t value = (vm_oop_t)valuePtr;
	((VMObject*) self)->SetField(fieldIndex, value);
}

int64_t
BytecodeHelper::getInvokable(int64_t receiverClazz, int64_t signature)
{
#define VALUE_FOR_GET_INVOKABLE_LINE LINETOSTR(__LINE__)

	VMClass* receiverClass = (VMClass *)receiverClazz;
	return (int64_t)receiverClass->LookupInvokable((VMSymbol *)signature);
}

int64_t
BytecodeHelper::doSendIfRequired(int64_t interp, int64_t framePtr, int64_t invokablePtr, int64_t receiverPtr, int64_t signaturePtr, int64_t bytecodeIndex)
{
#define VALUE_FOR_DO_SEND_IF_REQUIRED_LINE LINETOSTR(__LINE__)

	VMFrame *frame = (VMFrame *)framePtr;
	VMInvokable *invokable = (VMInvokable *)invokablePtr;
	Interpreter *interpreter = (Interpreter *)interp;
	interpreter->setBytecodeIndexGlobal((long)bytecodeIndex);
	frame->SetBytecodeIndex((long)bytecodeIndex);
	if (nullptr == invokable) {
		long numberOfArgs = Signature::GetNumberOfArguments((VMSymbol*)signaturePtr);
		vm_oop_t receiver = (vm_oop_t)receiverPtr;

		VMArray* argumentsArray = GetUniverse()->NewArray(numberOfArgs - 1); // without receiver

		// the receiver should not go into the argumentsArray
		// so, numberOfArgs - 2
		for (long i = numberOfArgs - 2; i >= 0; --i) {
			vm_oop_t o = frame->Pop();
			argumentsArray->SetIndexableField(i, o);
		}
		vm_oop_t arguments[] = {(VMSymbol*)signaturePtr, argumentsArray};

		frame->Pop(); // pop the receiver

		//check if current frame is big enough for this unplanned Send
		//doesNotUnderstand: needs 3 slots, one for this, one for method name, one for args
		long additionalStackSlots = 3 - frame->RemainingStackSize();
		if (additionalStackSlots > 0) {
			// copy current frame into a bigger one, and replace it
			interpreter->SetFrame(VMFrame::EmergencyFrameFrom(frame, additionalStackSlots));
		}

		AS_OBJ(receiver)->Send(interpreter, "doesNotUnderstand:arguments:", arguments, 2);
		Interpreter::runInterpreterLoop(interpreter);
		if (frame != interpreter->GetFrame()) {
			printf("Came back from a doesNotUnderstand and the frame was grown!!\n");
			/* TODO replace with a runtime assert */
			int *x = 0;
			*x = 0;
		}
	} else {
		invokable->Invoke(interpreter, frame);
		if (interpreter->GetReturnCount() > 0) {
			/* coming back from a JIT method on a return non local path */
			interpreter->DecrementReturnCount();
			frame->SetIsJITFrame(false);
			return -1;
		}

		if (GetHeap<HEAP_CLS>()->isCollectionTriggered()) {
			/* Do a GC at the same point the interpreter would have */
			interpreter->GetFrame()->SetBytecodeIndex(interpreter->getBytecodeIndexGlobal());
			GetHeap<HEAP_CLS>()->FullGC();
			/* TODO set frame, method, literals, sp, etc. */
			/* not needed yet because objects do not move??? */
		}

		if (frame != interpreter->GetFrame()) {
			interpreter->setBytecodeIndexGlobal(interpreter->GetFrame()->GetBytecodeIndex());
			Interpreter::runInterpreterLoop(interpreter);
			if (interpreter->GetReturnCount() > 0) {
				/* coming from a return non local */
				interpreter->DecrementReturnCount();
				frame->SetIsJITFrame(false);
				return -1;
			}
		}

		if (frame != interpreter->GetFrame()) {
			printf("Incorrect frame pointer in send. Frame %lx but interpreter->GetFrame() %lx\n", (int64_t)frame, (int64_t)interpreter->GetFrame());
			/* TODO replace with a runtime assert */
			int *x = 0;
			*x = 0;
		}
	}

	return interpreter->GetFrame()->GetBytecodeIndex();
}

int64_t
BytecodeHelper::doSuperSendHelper(int64_t interp, int64_t framePtr, int64_t signaturePtr, int64_t bytecodeIndex)
{
#define VALUE_FOR_DO_SUPER_SEND_HELPER_LINE LINETOSTR(__LINE__)

	VMFrame *frame = (VMFrame *)framePtr;
	VMSymbol* signature = (VMSymbol *)signaturePtr;
	VMFrame* ctxt = frame->GetOuterContext();
	VMMethod* realMethod = ctxt->GetMethod();
	VMClass* holder = realMethod->GetHolder();
	VMClass* super = holder->GetSuperClass();
	VMInvokable* invokable = static_cast<VMInvokable*>(super->LookupInvokable(signature));
	Interpreter *interpreter = (Interpreter *)interp;

	interpreter->setBytecodeIndexGlobal((long)bytecodeIndex);
	frame->SetBytecodeIndex((long)bytecodeIndex);

	if (invokable != nullptr) {
		invokable->Invoke(interpreter, frame);
		if (interpreter->GetReturnCount() > 0) {
			/* coming back from a JIT method on a return non local path */
			interpreter->DecrementReturnCount();
			frame->SetIsJITFrame(false);
			return -1;
		}

		if (GetHeap<HEAP_CLS>()->isCollectionTriggered()) {
			/* Do a GC at the same point the interpreter would have */
			interpreter->GetFrame()->SetBytecodeIndex(interpreter->getBytecodeIndexGlobal());
			GetHeap<HEAP_CLS>()->FullGC();
			/* TODO set frame, method, literals, sp, etc. */
			/* not needed yet because objects do not move??? */
		}


		if (frame != interpreter->GetFrame()) {
			interpreter->setBytecodeIndexGlobal(interpreter->GetFrame()->GetBytecodeIndex());
			Interpreter::runInterpreterLoop(interpreter);
			if (interpreter->GetReturnCount() > 0) {
				/* coming from a return non local */
				interpreter->DecrementReturnCount();
				frame->SetIsJITFrame(false);
				return -1;
			}
		}

		if (frame != interpreter->GetFrame()) {
			printf("Incorrect frame pointer in super send. Frame %lx but interpreter->GetFrame() %lx\n", (int64_t)frame, (int64_t)interpreter->GetFrame());
			/* TODO replace with a runtime assert */
			int *x = 0;
			*x = 0;
		}
	} else {
		long numOfArgs = Signature::GetNumberOfArguments(signature);
		vm_oop_t receiver = frame->GetStackElement(numOfArgs - 1);
		VMArray* argumentsArray = GetUniverse()->NewArray(numOfArgs);

		for (long i = numOfArgs - 1; i >= 0; --i) {
			vm_oop_t o = frame->Pop();
			argumentsArray->SetIndexableField(i, o);
		}
		vm_oop_t arguments[] = {signature, argumentsArray};

		AS_OBJ(receiver)->Send(interpreter, "doesNotUnderstand:arguments:", arguments, 2);
	}

	return interpreter->GetFrame()->GetBytecodeIndex();
}

void
BytecodeHelper::popFrameAndPushResult(int64_t interp, int64_t framePtr, int64_t result)
{
#define VALUE_FOR_POP_FRAME_AND_PUSH_RESULT_LINE LINETOSTR(__LINE__)

	Interpreter *interpreter = (Interpreter *)interp;
	/* Code from pop frame since it is private */
	VMFrame* prevFrame = interpreter->GetFrame();
	VMFrame *currentFrame = prevFrame->GetPreviousFrame();
	interpreter->SetFrame(currentFrame);
	prevFrame->ClearPreviousFrame();

	if (currentFrame->GetIsJITFrame() && !prevFrame->GetIsJITFrame()) {
		interpreter->IncrementReturnCount();
	}

	VMMethod* method = prevFrame->GetMethod();
	long numberOfArgs = method->GetNumberOfArguments();

	for (long i = 0; i < numberOfArgs; ++i) currentFrame->Pop();

	currentFrame->Push((vm_oop_t)result);
}

int64_t
BytecodeHelper::popToContext(int64_t interp, int64_t framePtr)
{
#define VALUE_FOR_POP_TO_CONTEXT_LINE LINETOSTR(__LINE__)

	Interpreter *interpreter = (Interpreter *)interp;
	VMFrame* currentFrame = (VMFrame *)framePtr;
	VMFrame *context = currentFrame->GetOuterContext();

	if (!context->HasPreviousFrame()) {
		return 1;
	}

	/* pop the first frame without counting because
	 * we are in the JIT and we will always return
	 */
	if (currentFrame != context) {
		/* Code from pop frame since it is private */
		VMFrame *previousFrame = currentFrame->GetPreviousFrame();
		interpreter->SetFrame(previousFrame);
		currentFrame->ClearPreviousFrame();
		currentFrame = previousFrame;
	}

	long interpreterFrameCounter = 0;
	while (currentFrame != context) {
		if (currentFrame->GetIsJITFrame()) {
			interpreter->IncrementReturnCount();
			if (interpreterFrameCounter > 0) {
				interpreter->IncrementReturnCount();
				interpreterFrameCounter = 0;
			}
		} else {
			interpreterFrameCounter += 1;
		}
		/* Code from pop frame since it is private */
		VMFrame *previousFrame = currentFrame->GetPreviousFrame();
		interpreter->SetFrame(previousFrame);
		currentFrame->ClearPreviousFrame();
		currentFrame = previousFrame;
	}

	if (context->GetIsJITFrame()) {
		interpreter->IncrementReturnCount();
		if (interpreterFrameCounter > 0) {
			interpreter->IncrementReturnCount();
			interpreterFrameCounter = 0;
		}
	}

	return 0;
}

const char* BytecodeHelper::GET_CLASS_LINE = VALUE_FOR_GET_CLASS_LINE;
const char* BytecodeHelper::GET_SUPER_CLASS_LINE = VALUE_FOR_GET_SUPER_CLASS_LINE;
const char* BytecodeHelper::GET_GLOBAL_LINE = VALUE_FOR_GET_GLOBAL_LINE;
const char* BytecodeHelper::GET_NEW_BLOCK_LINE = VALUE_FOR_GET_NEW_BLOCK_LINE;
const char* BytecodeHelper::NEW_INTEGER_LINE = VALUE_FOR_NEW_INTEGER_LINE;
const char* BytecodeHelper::NEW_DOUBLE_LINE = VALUE_FOR_NEW_DOUBLE_LINE;
const char* BytecodeHelper::GET_FIELD_FROM_LINE = VALUE_FOR_GET_FIELD_FROM_LINE;
const char* BytecodeHelper::SET_FIELD_TO_LINE = VALUE_FOR_SET_FIELD_TO_LINE;
const char* BytecodeHelper::GET_INVOKABLE_LINE = VALUE_FOR_GET_INVOKABLE_LINE;
const char* BytecodeHelper::DO_SEND_IF_REQUIRED_LINE = VALUE_FOR_DO_SEND_IF_REQUIRED_LINE;
const char* BytecodeHelper::DO_SUPER_SEND_HELPER_LINE = VALUE_FOR_DO_SUPER_SEND_HELPER_LINE;
const char* BytecodeHelper::POP_FRAME_AND_PUSH_RESULT_LINE = VALUE_FOR_POP_FRAME_AND_PUSH_RESULT_LINE;
const char* BytecodeHelper::POP_TO_CONTEXT_LINE = VALUE_FOR_POP_TO_CONTEXT_LINE;
