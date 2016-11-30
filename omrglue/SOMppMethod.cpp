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

#include "SOMppMethod.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <dlfcn.h>
#include <errno.h>

#include "Jit.hpp"
#include "ilgen/BytecodeBuilder.hpp"
#include "ilgen/MethodBuilder.hpp"
#include "ilgen/TypeDictionary.hpp"
#include "ilgen/VirtualMachineOperandStack.hpp"
#include "ilgen/VirtualMachineRegisterInStruct.hpp"
#include "vmobjects/Signature.h"
#include "vmobjects/VMFrame.h"
#include "vmobjects/VMMethod.h"
#include "vmobjects/VMSymbol.h"
#include "interpreter/bytecodes.h"
#include "interpreter/Interpreter.h"

#include "BytecodeHelper.hpp"

#define DO_DEBUG_PRINTS 0

#if DEBUG
#define SOM_METHOD_DEBUG true
#elif 1 == DO_DEBUG_PRINTS
#define SOM_METHOD_DEBUG true
#else
#undef SOM_METHOD_DEBUG
#endif

class SOMppVMState : public OMR::VirtualMachineState
   {
   public:
	SOMppVMState()
      : OMR::VirtualMachineState(),
      _stack(NULL),
      _stackTop(NULL)
      { }

	SOMppVMState(OMR::VirtualMachineOperandStack *stack, OMR::VirtualMachineRegister *stackTop)
      : OMR::VirtualMachineState(),
      _stack(stack),
      _stackTop(stackTop)
      {
      }

   virtual void Commit(TR::IlBuilder *b)
      {
      _stack->Commit(b);
      _stackTop->Commit(b);
      }

   virtual void Reload(TR::IlBuilder *b)
      {
      _stack->Reload(b);
      _stackTop->Reload(b);
      }

   virtual VirtualMachineState *MakeCopy()
      {
	  SOMppVMState *newState = new SOMppVMState();
      newState->_stack = (OMR::VirtualMachineOperandStack *)_stack->MakeCopy();
      newState->_stackTop = (OMR::VirtualMachineRegister *) _stackTop->MakeCopy();
      return newState;
      }

   virtual void MergeInto(VirtualMachineState *other, TR::IlBuilder *b)
      {
	   SOMppVMState *otherState = (SOMppVMState *)other;
      _stack->MergeInto(otherState->_stack, b);
      _stackTop->MergeInto(otherState->_stackTop, b);
      }

   OMR::VirtualMachineOperandStack * _stack;
   OMR::VirtualMachineRegister     * _stackTop;
   };

#define STACK(b)	    (((SOMppVMState *)(b)->vmState())->_stack)
#define STACKTOP(b)	    ((SOMppVMState *)(b)->vmState())->_stackTop)
#define COMMIT(b)       ((b)->vmState()->Commit(b))
#define RELOAD(b)       ((b)->vmState()->Reload(b))
#define PUSH(b,v)	    (STACK(b)->Push(b,v))
#define POP(b)          (STACK(b)->Pop(b))
#define TOP(b)          (STACK(b)->Top())
#define DUP(b)          (STACK(b)->Dup(b))
#define DROP(b,d)       (STACK(b)->Drop(b,d))
#define DROPALL(b)    (STACK(b)->DropAll(b))
#define PICK(b,d)       (STACK(b)->Pick(d))

static void
printString(int64_t stringPointer)
{
#define PRINTSTRING_LINE LINETOSTR(__LINE__)
	char *strPtr = (char *) stringPointer;
	fprintf(stderr, "%s", strPtr);
}

static void
printInt64(int64_t value)
{
#define PRINTINT64_LINE LINETOSTR(__LINE__)
	fprintf(stderr, "%ld", value);
}

static void
printInt64Hex(int64_t value)
{
#define PRINTINT64HEX_LINE LINETOSTR(__LINE__)
	fprintf(stderr, "%lx", value);
}

SOMppMethod::SOMppMethod(TR::TypeDictionary *types, VMMethod *vmMethod, bool inlineCalls) :
		MethodBuilder(types),
		method(vmMethod),
		doInlining(inlineCalls),
		currentStackDepth(0)
{
	DefineLine(LINETOSTR(__LINE__));
	DefineFile(__FILE__);

	char *signature = method->GetSignature()->GetChars();
	char *holder = method->GetHolder()->GetName()->GetChars();

	snprintf(methodName, 64, "%s>>#%s", holder, signature);

	fieldNames[0] = "field0";
	fieldNames[1] = "field1";
	fieldNames[2] = "field2";
	fieldNames[3] = "field3";
	fieldNames[4] = "field4";
	fieldNames[5] = "field5";
	fieldNames[6] = "field6";
	fieldNames[7] = "field7";
	fieldNames[8] = "field8";
	fieldNames[9] = "field9";

	DefineName(methodName);
	DefineReturnType(NoType);

	defineStructures(types);
	defineParameters();
	defineLocals();
	defineFunctions();

	AllLocalsHaveBeenDefined();
}

void
SOMppMethod::defineParameters()
{
	DefineParameter("interpreter", Int64);
	DefineParameter("frame", pVMFrame);
}

void
SOMppMethod::defineLocals()
{
	/* specific for Integer operations */
	DefineLocal("leftValueInteger", Int64);
	DefineLocal("rightValueInteger", Int64);
	DefineLocal("integerValue", Int64);

	/* Specific for Double operations */
	DefineLocal("rightObjectDouble", Int64);
	DefineLocal("rightClassDouble", Int64);
	DefineLocal("rightValueSlotDouble", Int64);
	DefineLocal("rightValueDouble", Double);
	DefineLocal("leftObjectDouble", Int64);
	DefineLocal("leftClassDouble", Int64);
	DefineLocal("leftValueSlotDouble", Int64);
	DefineLocal("leftValueDouble", Double);
}

void
SOMppMethod::defineStructures(TR::TypeDictionary *types)
{
	pInt64 = types->PointerTo(Int64);
	pDouble = types->PointerTo(Double);

	valueType = STACKVALUEILTYPE;

	defineVMFrameStructure(types);
	defineVMObjectStructure(types);
}

void
SOMppMethod::defineFunctions()
{
	DefineFunction((char *)"printString", (char *)__FILE__, (char *)PRINTSTRING_LINE, (void *)&printString, NoType, 1, Int64);
	DefineFunction((char *)"printInt64", (char *)__FILE__, (char *)PRINTINT64_LINE, (void *)&printInt64, NoType, 1, Int64);
	DefineFunction((char *)"printInt64Hex", (char *)__FILE__, (char *)PRINTINT64HEX_LINE, (void *) &printInt64Hex, NoType, 1, Int64);
	DefineFunction((char *)"getClass", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::GET_CLASS_LINE, (void *)&BytecodeHelper::getClass, Int64, 1, Int64);
	DefineFunction((char *)"getGlobal", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::GET_GLOBAL_LINE, (void *)&BytecodeHelper::getGlobal, Int64, 1, Int64);
	DefineFunction((char *)"getNewBlock", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::GET_NEW_BLOCK_LINE, (void *)&BytecodeHelper::getNewBlock, Int64, 3, Int64, Int64, Int64);
	DefineFunction((char *)"newInteger", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::NEW_INTEGER_LINE, (void *)&BytecodeHelper::newInteger, Int64, 1, Int64);
	DefineFunction((char *)"newDouble", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::NEW_DOUBLE_LINE, (void *)&BytecodeHelper::newDouble, Int64, 1, Double);
	DefineFunction((char *)"getFieldFrom", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::GET_FIELD_FROM_LINE, (void *)&BytecodeHelper::getFieldFrom, Int64, 2, Int64, Int64);
	DefineFunction((char *)"setFieldTo", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::SET_FIELD_TO_LINE, (void *)&BytecodeHelper::setFieldTo, NoType, 3, Int64, Int64, Int64);
	DefineFunction((char *)"getInvokable", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::GET_INVOKABLE_LINE, (void *)&BytecodeHelper::getInvokable, Int64, 2, Int64, Int64);
	DefineFunction((char *)"doSendIfRequired", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::DO_SEND_IF_REQUIRED_LINE, (void *)&BytecodeHelper::doSendIfRequired, Int64, 4, Int64, Int64, Int64, Int64);
	DefineFunction((char *)"doSuperSendHelper", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::DO_SUPER_SEND_HELPER_LINE, (void *)&BytecodeHelper::doSuperSendHelper, Int64, 4, Int64, Int64, Int64, Int64);
	DefineFunction((char *)"popFrameAndPushResult", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::POP_FRAME_AND_PUSH_RESULT_LINE, (void *)&BytecodeHelper::popFrameAndPushResult, NoType, 3, Int64, Int64, Int64);
	DefineFunction((char *)"popToContext", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::POP_TO_CONTEXT_LINE, (void *)&BytecodeHelper::popToContext, Int64, 2, Int64, Int64);
}

void
SOMppMethod::defineVMFrameStructure(TR::TypeDictionary *types)
{
	vmFrame = types->DefineStruct("VMFrame");

	pVMFrame = types->PointerTo("VMFrame");

	types->DefineField("VMFrame", "vTable", Int64);
	types->DefineField("VMFrame", "gcField", Int64);
	types->DefineField("VMFrame", "hash", Int64);
	types->DefineField("VMFrame", "objectSize", Int64);
	types->DefineField("VMFrame", "numberOfFields", Int64);
	types->DefineField("VMFrame", "clazz", Int64);
	types->DefineField("VMFrame", "previousFrame", Int64);
	types->DefineField("VMFrame", "context", Int64);
	types->DefineField("VMFrame", "method", Int64);
	types->DefineField("VMFrame", "isJITFrame", Int64);
	types->DefineField("VMFrame", "bytecodeIndex", Int64);
	types->DefineField("VMFrame", "arguments", Int64);
	types->DefineField("VMFrame", "locals", Int64);
	types->DefineField("VMFrame", "stack_ptr", pInt64);
	types->CloseStruct("VMFrame");
}

void
SOMppMethod::defineVMObjectStructure(TR::TypeDictionary *types)
{
	vmObject = types->DefineStruct("VMObject");

	types->DefineField("VMObject", "vTable", Int64);
	types->DefineField("VMObject", "gcField", Int64);
	types->DefineField("VMObject", "hash", Int64);
	types->DefineField("VMObject", "objectSize", Int64);
	types->DefineField("VMObject", "numberOfFields", Int64);
	types->DefineField("VMObject", "clazz", Int64);
	for (int i = 0; i < FIELDNAMES_LENGTH; i++) {
		types->DefineField("VMObject", fieldNames[i], Int64);
	}
	types->CloseStruct("VMObject");
}

int64_t
SOMppMethod::calculateBytecodeIndexForJump(long bytecodeIndex)
{
	int64_t target = 0;
    target |= method->GetBytecode(bytecodeIndex + 1);
    target |= method->GetBytecode(bytecodeIndex + 2) << 8;
    target |= method->GetBytecode(bytecodeIndex + 3) << 16;
    target |= method->GetBytecode(bytecodeIndex + 4) << 24;

    return target;
}

void
SOMppMethod::createBuilderForBytecode(TR::BytecodeBuilder **bytecodeBuilderTable, uint8_t bytecode, int64_t bytecodeIndex)
{
	TR::BytecodeBuilder *newBuilder = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(bytecode));
	bytecodeBuilderTable[bytecodeIndex] = newBuilder;
}

void
SOMppMethod::justReturn(TR::IlBuilder *from)
{
	from->Return(
	from->   ConstInt64(3));
}

bool
SOMppMethod::buildIL()
{
	TR::BytecodeBuilder **bytecodeBuilderTable = nullptr;
	bool canHandle = true;
	int64_t i = 0;
	doInlining = false;

#if SOM_METHOD_DEBUG
	printf("\nGenerating code for %s\n", methodName);
#endif

	stackTop = new OMR::VirtualMachineRegisterInStruct(this, "VMFrame", "frame", "stack_ptr", "SP");
	stack = new OMR::VirtualMachineOperandStack(this, 32, valueType, stackTop);

	SOMppVMState *vmState = new SOMppVMState(stack, stackTop);
	setVMState(vmState);

	long numOfBytecodes = method->GetNumberOfBytecodes();
	long tableSize = sizeof(TR::BytecodeBuilder *) * numOfBytecodes;
	bytecodeBuilderTable = (TR::BytecodeBuilder **)malloc(tableSize);
	if (NULL == bytecodeBuilderTable) {
		return false;
	}

	memset(bytecodeBuilderTable, 0, tableSize);

	i = 0;
	while (i < numOfBytecodes) {
		uint8_t bc = method->GetBytecode(i);
		createBuilderForBytecode(bytecodeBuilderTable, bc, i);
		i += Bytecode::GetBytecodeLength(bc);
	}

	AppendBuilder(bytecodeBuilderTable[0]);

	i = 0;
	while (i < numOfBytecodes) {
		uint8_t bc = method->GetBytecode(i);
#if SOM_METHOD_DEBUG
		printf("\tbytcode %s index %ld started ...", Bytecode::GetBytecodeName(bc), i);
#endif
		if (!generateILForBytecode(bytecodeBuilderTable, bc, i)) {
			canHandle = false;
			break;
		}
#if SOM_METHOD_DEBUG
		printf("\tfinished\n");
#endif
		i += Bytecode::GetBytecodeLength(bc);
	}

	free((void *)bytecodeBuilderTable);
	return canHandle;
}

/*************************************************
 * GENERATE CODE FOR BYTECODES
 *************************************************/

void
SOMppMethod::doDup(TR::BytecodeBuilder *builder)
{
	DUP(builder);
	currentStackDepth += 1;
}

void
SOMppMethod::doPushLocal(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	uint8_t index = method->GetBytecode(bytecodeIndex + 1);
	uint8_t level = method->GetBytecode(bytecodeIndex + 2);
	const char *contextName = getContext(builder, level);

	TR::IlValue *local =
	builder->LoadAt(pInt64,
	builder->	IndexAt(pInt64,
	builder->		LoadIndirect("VMFrame", "locals",
	builder->			Load(contextName)),
	builder->		ConstInt64(index)));

	PUSH(builder, local);

	currentStackDepth += 1;
}

void
SOMppMethod::doPushArgument(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	uint8_t index = method->GetBytecode(bytecodeIndex + 1);
	uint8_t level = method->GetBytecode(bytecodeIndex + 2);
	const char *contextName = getContext(builder, level);

	TR::IlValue *argument =
	builder->LoadAt(pInt64,
	builder->	IndexAt(pInt64,
	builder->		LoadIndirect("VMFrame", "arguments",
	builder->			Load(contextName)),
	builder->		ConstInt64(index)));

	PUSH(builder, argument);

	currentStackDepth += 1;
}

void
SOMppMethod::doPushField(TR::BytecodeBuilder *builder, VMMethod *currentMethod, long bytecodeIndex)
{
	uint8_t fieldIndex = method->GetBytecode(bytecodeIndex + 1);

	TR::IlValue *outerContext = getOuterContext(builder);
	TR::IlValue *object = getSelfFromContext(builder, outerContext);
	TR::IlValue *field = nullptr;

	if (fieldIndex < FIELDNAMES_LENGTH) {
		const char *fieldName = fieldNames[fieldIndex];
		field =
		builder->LoadIndirect("VMObject", fieldName, object);
	} else {
		field =
		builder->Call("getFieldFrom", 2, object,
		builder->	ConstInt64((int64_t)fieldIndex));
	}

	PUSH(builder, field);

	currentStackDepth += 1;
}

void
SOMppMethod::doPushBlock(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	/* TODO come back and handle optimization in Interpreter::doPushBlock */
	VMMethod* blockMethod = static_cast<VMMethod*>(method->GetConstant(bytecodeIndex));
	long numOfArgs = blockMethod->GetNumberOfArguments();

	TR::IlValue *block =
	builder->Call("getNewBlock", 3,
	builder->	Load("frame"),
	builder->	ConstInt64((int64_t)blockMethod),
	builder->	ConstInt64((int64_t)numOfArgs));

	PUSH(builder, block);

	currentStackDepth += 1;
}

void
SOMppMethod::doPushConstant(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	uint8_t valueOffset = method->GetBytecode(bytecodeIndex + 1);

	TR::IlValue *constant =
	builder->LoadAt(pInt64,
	builder->	IndexAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			ConstInt64((int64_t)method->indexableFields)),
	builder->		ConstInt64(valueOffset)));

	PUSH(builder, constant);

	currentStackDepth += 1;
}

void
SOMppMethod::doPushGlobal(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	/* TODO If objects can move this is a runtime and not compile time fetch */
	VMSymbol* globalName = static_cast<VMSymbol*>(method->GetConstant(bytecodeIndex));

	TR::IlValue *global =
	builder->	Call("getGlobal", 1,
	builder->		ConstInt64((int64_t)globalName));

	TR::IlBuilder *globalIsNullPtr = NULL;
	builder->IfThen(&globalIsNullPtr,
	builder->	EqualTo(global,
	builder->		ConstInt64((int64_t)nullptr)));

	/* TODO Come back and handle */
	globalIsNullPtr->Call("printString", 1,
	globalIsNullPtr->	ConstInt64((int64_t)"\n\n\n doPushGlobal crashing due to unknown global\n\n\n\n"));
	globalIsNullPtr->StoreAt(
	globalIsNullPtr->	ConvertTo(pInt64,
	globalIsNullPtr->		ConstInt64(0)),
	globalIsNullPtr->	ConstInt64(0));

	justReturn(globalIsNullPtr);

	PUSH(builder, global);

	currentStackDepth += 1;
}

void
SOMppMethod::doPop(TR::BytecodeBuilder *builder)
{
	POP(builder);
	currentStackDepth -= 1;
}

void
SOMppMethod::doPopLocal(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	uint8_t index = method->GetBytecode(bytecodeIndex + 1);
	uint8_t level = method->GetBytecode(bytecodeIndex + 2);

	TR::IlValue *value = POP(builder);

	const char* contextName = getContext(builder, level);

	builder->StoreAt(
	builder->	IndexAt(pInt64,
	builder->		LoadIndirect("VMFrame", "locals",
	builder->			Load(contextName)),
	builder->		ConstInt64(index)), value);

	currentStackDepth -= 1;
}

void
SOMppMethod::doPopArgument(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	uint8_t index = method->GetBytecode(bytecodeIndex + 1);
	/* see Interpreter::doPopArgument and VMFrame::SetArgument.
	 * level does not appear to be used.
	uint8_t level = method->GetBytecode(bytecodeIndex + 2);
	*/

	TR::IlValue *value = POP(builder);

	/*
	 * See comment above about level not being used
	builder->Store("context",
	builder->	Load("frame"));

	if (level > 0) {
		TR::IlBuilder *iloop = NULL;
		builder->ForLoopUp("i", &iloop,
		builder->	ConstInt32(0),
		builder->	ConstInt32(level),
		builder->	ConstInt32(1));

		iloop->Store("context",
		iloop->	LoadIndirect("VMFrame", "context",
		iloop->		Load("context")));
	}
	*/

	builder->StoreAt(
	builder->	IndexAt(pInt64,
	builder->		LoadIndirect("VMFrame", "arguments",
	builder->			Load("frame")),
	builder->		ConstInt64(index)), value);

	currentStackDepth -= 1;
}

void
SOMppMethod::doPopField(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	uint8_t fieldIndex = method->GetBytecode(bytecodeIndex + 1);

	TR::IlValue *outerContext = getOuterContext(builder);
	TR::IlValue *object = getSelfFromContext(builder, outerContext);
	TR::IlValue *value = POP(builder);

	if (fieldIndex < FIELDNAMES_LENGTH) {
		const char *fieldName = fieldNames[fieldIndex];
		builder->StoreIndirect("VMObject", fieldName, object, value);
	} else {
		builder->Call("setFieldTo", 3, object,
		builder->	ConstInt64((int64_t)fieldIndex), value);
	}

	currentStackDepth -= 1;
}

void
SOMppMethod::doSend(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex, TR::BytecodeBuilder *fallThrough)
{
	VMSymbol* signature = static_cast<VMSymbol*>(method->GetConstant(bytecodeIndex));
	int numOfArgs = Signature::GetNumberOfArguments(signature);

	builder->Store("receiverClass",
	builder->	Call("getClass", 1, PICK(builder, numOfArgs - 1)));

	/******************************************
	 * START of inlining code which will move into a helpers once I figure out how to do it
	 */

	/* Toggle this boolean to try to do inlining */
	bool attemptInline = true;
	VMClass *receiverFromCache = method->getInvokeReceiverCache(bytecodeIndex);
	char *signatureChars = signature->GetChars();
	bool didInline = false;

	TR::BytecodeBuilder *genericSend = NULL;
	TR::BytecodeBuilder *merge = NULL;

	if (attemptInline && (VMClass*)integerClass == receiverFromCache) {
		if (0 == strcmp("+", signatureChars)) {

			printf("\n\n\n\n in here \n\n\n\n");
			assert(numOfArgs == 2);

			genericSend = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
			merge = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
			TR::IlValue *receiver = PICK(builder, 1);
			TR::IlValue *param1 = TOP(builder);

			/* if the receiver is not low tagged then bail */
			builder->IfCmpEqualZero(&genericSend,
			builder->	And(receiver,
			builder->		ConstInt64(0x0)));

			/* if the value is not low tagged then bail */
			builder->IfCmpEqualZero(&genericSend,
			builder->	And(param1,
			builder->		ConstInt64(0x0)));

			builder->Store("receiverValue",
			builder->	ShiftR(receiver,
			builder->	ConstInt64(1)));

			builder->Store("param1Value",
			builder->	ShiftR(param1,
			builder->	ConstInt64(1)));

			/* do the addition */
			TR::IlValue *newValue =
			builder->Add(
			builder->	Load("receiverValue"),
			builder->	Load("param1Value"));

			TR::IlBuilder *lessThanMin = nullptr;
			TR::IlBuilder *possibleForTagging = nullptr;
			builder->IfThenElse(&lessThanMin, &possibleForTagging,
			builder->	GreaterThan(
			builder->		ConstInt64((int64_t)VMTAGGEDINTEGER_MIN), newValue));

			lessThanMin->Store("newInteger",
			lessThanMin->	Call("newInteger", 1, newValue));

			TR::IlBuilder *greaterThanMax = nullptr;
			TR::IlBuilder *tag = nullptr;
			possibleForTagging->IfThenElse(&greaterThanMax, &tag,
			possibleForTagging->	LessThan(
			possibleForTagging->		ConstInt64((int64_t)VMTAGGEDINTEGER_MAX), newValue));

			greaterThanMax->Store("newInteger",
			greaterThanMax->	Call("newInteger", 1, newValue));

			tag->Store("shiftedValue",
			tag->	ShiftL(newValue,
			tag->		ConstInt64(1)));
			tag->Store("newInteger",
			tag->	Add(
			tag->		Load("shiftedValue"),
			tag->		ConstInt64(1)));

			builder->Store("sendResult",
			builder->	Load("newInteger"));

			didInline = true;
		}
	}

	if (didInline) {
		builder->Goto(merge);
		builder->AppendBuilder(genericSend);
		builder->AppendBuilder(merge);


	} else {
		genericSend = builder;
		merge = builder;
	}

	/**************************************************
	 * END of inling code
	 */

	/* going to call out of line helper so commit the stack */
	COMMIT(genericSend);

	genericSend->Store("invokable",
	genericSend->	Call("getInvokable", 2,
	genericSend->		Load("receiverClass"),
	genericSend->		ConstInt64((int64_t)signature)));

	genericSend->Store("return",
	genericSend->	Call("doSendIfRequired", 4,
	genericSend->		Load("interpreter"),
	genericSend->		Load("frame"),
	genericSend->		Load("invokable"),
	genericSend->		ConstInt64((int64_t)bytecodeIndex)));

	TR::IlBuilder *bail = NULL;
	genericSend->IfThen(&bail,
	genericSend->	EqualTo(
	genericSend->		Load("return"),
	genericSend->		ConstInt64(-1)));

	justReturn(bail);

	genericSend->Store("sendResult",
	genericSend->	LoadAt(pInt64,
	genericSend->		LoadIndirect("VMFrame", "stack_ptr",
	genericSend->			Load("frame"))));

	DROP(merge, numOfArgs);

	TR::BytecodeBuilder *restartIfRequired = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	merge->IfCmpNotEqual(&restartIfRequired,
	merge->	Load("return"),
	merge->	ConstInt64((int64_t)bytecodeIndex));

	DROPALL(restartIfRequired);

	TR::BytecodeBuilder *start = bytecodeBuilderTable[0];
	restartIfRequired->Goto(start);

	PUSH(merge, merge->Load("sendResult"));

	merge->AddFallThroughBuilder(fallThrough);

	currentStackDepth -= (numOfArgs - 1);
}

void
SOMppMethod::doSuperSend(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex)
{
	VMSymbol* signature = static_cast<VMSymbol*>(method->GetConstant(bytecodeIndex));
	int numOfArgs = Signature::GetNumberOfArguments(signature);

#if SOM_METHOD_DEBUG
	char *signatureChars = signature->GetChars();
	printf(" doSuperSend to %s with inlining %d ", signatureChars, (int)doInlining);
#endif

	COMMIT(builder);

	builder->Store("return",
	builder->	Call("doSuperSendHelper", 4,
	builder->		Load("interpreter"),
	builder->		Load("frame"),
	builder->		ConstInt64((int64_t)signature),
	builder->		ConstInt64((int64_t)bytecodeIndex)));

	TR::IlBuilder *bail = NULL;
	builder->IfThen(&bail,
	builder->   EqualTo(
	builder->		Load("return"),
	builder->		ConstInt64(-1)));

	justReturn(bail);

	DROP(builder, numOfArgs);

	TR::BytecodeBuilder *restartIfRequired = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	builder->IfCmpNotEqual(&restartIfRequired,
	builder->	Load("return"),
	builder->	ConstInt64((int64_t)bytecodeIndex));

	DROPALL(restartIfRequired);

	TR::BytecodeBuilder *start = bytecodeBuilderTable[0];
	restartIfRequired->Goto(start);

	TR::IlValue *value =
	builder->LoadAt(pInt64,
	builder->	LoadIndirect("VMFrame", "stack_ptr",
	builder->		Load("frame")));

	PUSH(builder, value);

	currentStackDepth -= (numOfArgs - 1);
}

void
SOMppMethod::doReturnLocal(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	TR::IlValue *result = POP(builder);

	/* there is no need to commit the stack as it is not used */

	builder->Call("popFrameAndPushResult", 3,
	builder->	Load("interpreter"),
	builder->	Load("frame"), result);

	justReturn(builder);
	/* do not adjust currentStackDepth */
}

void
SOMppMethod::doReturnNonLocal(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	TR::IlValue *result = POP(builder);

	builder->Store("return",
	builder->	Call("popToContext", 2,
	builder->		Load("interpreter"),
	builder->		Load("frame")));

	TR::IlBuilder *continuePath = NULL;
	TR::IlBuilder *didEscapedSend = NULL;

	builder->IfThenElse(&continuePath, &didEscapedSend,
	builder->	EqualTo(
	builder->		Load("return"),
	builder->		ConstInt64(0)));

	didEscapedSend->Call("printString", 1,
	didEscapedSend->	ConstInt64((int64_t)"\n\n\n doReturnNonLocal crashing due to escapedBlock\n\n\n\n"));
	didEscapedSend->StoreAt(
	didEscapedSend->	ConvertTo(pInt64,
	didEscapedSend->		ConstInt64(0)),
	didEscapedSend->	ConstInt64(0));

	continuePath->Call("popFrameAndPushResult", 3,
	continuePath->	Load("interpreter"),
	continuePath->	Load("frame"), result);

	justReturn(builder);
	/* do not adjust currentStackDepth */
}

void
SOMppMethod::doJumpIfFalse(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex)
{
#if SOM_METHOD_DEBUG
	printf(" jump to %lu ", calculateBytecodeIndexForJump(bytecodeIndex));
#endif

	TR::IlValue *value = POP(builder);

	TR::BytecodeBuilder *destBuilder = bytecodeBuilderTable[calculateBytecodeIndexForJump(bytecodeIndex)];
	builder->IfCmpEqual(&destBuilder, value,
	builder->	ConstInt64((int64_t)falseObject));

	currentStackDepth -= 1;
}

void
SOMppMethod::doJumpIfTrue(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex)
{
#if SOM_METHOD_DEBUG
	printf(" jump to %lu ", calculateBytecodeIndexForJump(bytecodeIndex));
#endif

	TR::IlValue *value = POP(builder);

	TR::BytecodeBuilder *destBuilder = bytecodeBuilderTable[calculateBytecodeIndexForJump(bytecodeIndex)];
	builder->IfCmpEqual(&destBuilder, value,
	builder->	ConstInt64((int64_t)trueObject));

	currentStackDepth -= 1;
}

void
SOMppMethod::doJump(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex)
{
#if SOM_METHOD_DEBUG
	printf(" jump to %lu ", calculateBytecodeIndexForJump(bytecodeIndex));
#endif

	TR::BytecodeBuilder *destBuilder = bytecodeBuilderTable[calculateBytecodeIndexForJump(bytecodeIndex)];
	builder->Goto(destBuilder);
	/* do not adjust currentStackDepth */
}

bool
SOMppMethod::generateILForBytecode(TR::BytecodeBuilder **bytecodeBuilderTable, uint8_t bytecode, long bytecodeIndex)
{
	TR::BytecodeBuilder *builder = bytecodeBuilderTable[bytecodeIndex];
#ifdef SOM_METHOD_DEBUG
	printf("builder %lx ", (uint64_t)builder);
#endif

	if (NULL == builder) {
		printf("unexpected NULL BytecodeBuilder!\n");
		return false;
	}

	TR::BytecodeBuilder *nextBytecodeBuilder = nullptr;
	long nextBytecodeIndex = bytecodeIndex + Bytecode::GetBytecodeLength(bytecode);
	long numOfBytecodes = method->GetNumberOfBytecodes();
	if (nextBytecodeIndex < numOfBytecodes) {
		nextBytecodeBuilder = bytecodeBuilderTable[nextBytecodeIndex];
	}

	if (NULL == builder->vmState()) {
		return true;
	}

	bool canHandle = true;

	switch(bytecode) {
	case BC_HALT:
		canHandle = false;
		break;
	case BC_DUP:
		doDup(builder);
		builder->AddFallThroughBuilder(nextBytecodeBuilder);
		break;
	case BC_PUSH_LOCAL:
		doPushLocal(builder, bytecodeIndex);
		builder->AddFallThroughBuilder(nextBytecodeBuilder);
		break;
	case BC_PUSH_ARGUMENT:
		doPushArgument(builder, bytecodeIndex);
		builder->AddFallThroughBuilder(nextBytecodeBuilder);
		break;
	case BC_PUSH_FIELD:
		doPushField(builder, method, bytecodeIndex);
		builder->AddFallThroughBuilder(nextBytecodeBuilder);
		break;
	case BC_PUSH_BLOCK:
		doPushBlock(builder, bytecodeIndex);
		builder->AddFallThroughBuilder(nextBytecodeBuilder);
		break;
	case BC_PUSH_CONSTANT:
		doPushConstant(builder, bytecodeIndex);
		builder->AddFallThroughBuilder(nextBytecodeBuilder);
		break;
	case BC_PUSH_GLOBAL:
		doPushGlobal(builder, bytecodeIndex);
		builder->AddFallThroughBuilder(nextBytecodeBuilder);
		break;
	case BC_POP:
		doPop(builder);
		builder->AddFallThroughBuilder(nextBytecodeBuilder);
		break;
	case BC_POP_LOCAL:
		doPopLocal(builder, bytecodeIndex);
		builder->AddFallThroughBuilder(nextBytecodeBuilder);
		break;
	case BC_POP_ARGUMENT:
		doPopArgument(builder, bytecodeIndex);
		builder->AddFallThroughBuilder(nextBytecodeBuilder);
		break;
	case BC_POP_FIELD:
		doPopField(builder, bytecodeIndex);
		builder->AddFallThroughBuilder(nextBytecodeBuilder);
		break;
	case BC_SEND:
		doSend(builder, bytecodeBuilderTable, bytecodeIndex, nextBytecodeBuilder);
//		builder->AddFallThroughBuilder(nextBytecodeBuilder);
		break;
	case BC_SUPER_SEND:
		doSuperSend(builder, bytecodeBuilderTable, bytecodeIndex);
		builder->AddFallThroughBuilder(nextBytecodeBuilder);
		break;
	case BC_RETURN_LOCAL:
		doReturnLocal(builder, bytecodeIndex);
		break;
	case BC_RETURN_NON_LOCAL:
		doReturnNonLocal(builder, bytecodeIndex);
		break;
	case BC_JUMP_IF_FALSE:
		doJumpIfFalse(builder, bytecodeBuilderTable, bytecodeIndex);
		builder->AddFallThroughBuilder(nextBytecodeBuilder);
		break;
	case BC_JUMP_IF_TRUE:
		doJumpIfTrue(builder, bytecodeBuilderTable, bytecodeIndex);
		builder->AddFallThroughBuilder(nextBytecodeBuilder);
		break;
	case BC_JUMP:
		doJump(builder, bytecodeBuilderTable, bytecodeIndex);
		break;
	default:
		canHandle = false;
	}
	return canHandle;
}

const char *
SOMppMethod::getContext(TR::IlBuilder *builder, uint8_t level)
{
	if (level == 0) {
		return "frame";
	} else {
		builder->Store("context",
		builder->	Load("frame"));

		if (level == 1) {
			builder->Store("context",
			builder->	LoadIndirect("VMFrame", "context",
			builder->		Load("context")));
		} else {
			TR::IlBuilder *iloop = NULL;
			builder->ForLoopUp("i", &iloop,
			builder->	ConstInt32(0),
			builder->	ConstInt32(level),
			builder->	ConstInt32(1));

			iloop->Store("context",
			iloop->	LoadIndirect("VMFrame", "context",
			iloop->		Load("context")));
		}
		return "context";
	}
}

TR::IlValue *
SOMppMethod::getOuterContext(TR::IlBuilder *builder)
{
	builder->Store("next",
	builder->	Load("frame"));

	builder->Store("contextNotNull",
	builder->	NotEqualTo(
	builder->		Load("next"),
	builder->		NullAddress()));

	TR::IlBuilder *loop = nullptr;
	builder->DoWhileLoop("contextNotNull", &loop);

	loop->Store("outerContext",
	loop->	Load("next"));
	loop->Store("next",
	loop->	LoadIndirect("VMFrame", "context",
	loop->		Load("outerContext")));
	loop->Store("contextNotNull",
	loop->	NotEqualTo(
	loop->		Load("next"),
	loop->      NullAddress()));

	return builder->Load("outerContext");
}

TR::IlValue *
SOMppMethod::getSelfFromContext(TR::IlBuilder *builder, TR::IlValue *context)
{
	TR::IlValue *object =
	builder->LoadAt(pInt64,
	builder->	IndexAt(pInt64,
	builder->		LoadIndirect("VMFrame", "arguments", context),
	builder->		ConstInt64(0)));

	return object;
}
