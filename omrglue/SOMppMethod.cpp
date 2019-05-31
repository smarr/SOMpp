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

//#include "Jit.hpp"
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

#define DO_DEBUG_PRINTS 1

#if DEBUG
#define SOM_METHOD_DEBUG true
#elif 1 == DO_DEBUG_PRINTS
#define SOM_METHOD_DEBUG true
#else
#undef SOM_METHOD_DEBUG
#endif

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

SOMppMethod::SOMppMethod(OMR::JitBuilder::TypeDictionary *types, VMMethod *vmMethod, bool inlineCalls) :
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
SOMppMethod::defineStructures(OMR::JitBuilder::TypeDictionary *types)
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
SOMppMethod::defineVMFrameStructure(OMR::JitBuilder::TypeDictionary *types)
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
SOMppMethod::defineVMObjectStructure(OMR::JitBuilder::TypeDictionary *types)
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
SOMppMethod::createBuilderForBytecode(OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable, uint8_t bytecode, int64_t bytecodeIndex)
{
  OMR::JitBuilder::BytecodeBuilder *newBuilder = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(bytecode)); //stack, bytecodeIndex, Bytecode::GetBytecodeName(bytecode));
  bytecodeBuilderTable[bytecodeIndex] = newBuilder;
}

void
SOMppMethod::justReturn(OMR::JitBuilder::IlBuilder *from)
{
	from->Return(
	from->   ConstInt64(3));
}

bool
SOMppMethod::buildIL()
{
	OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable = nullptr;
	bool canHandle = true;
	int64_t i = 0;

#if SOM_METHOD_DEBUG
	printf("\nGenerating code for %s\n", methodName);
#endif

	stackTop = new OMR::JitBuilder::VirtualMachineRegisterInStruct(this, "VMFrame", "frame", "stack_ptr", "SP");
#if SOM_METHOD_DEBUG
	printf("\t stacktop created\n");
#endif
	stack = new OMR::JitBuilder::VirtualMachineOperandStack(this, 32, valueType, stackTop);

#if SOM_METHOD_DEBUG
	printf("\t stack created\n");
#endif

	long numOfBytecodes = method->GetNumberOfBytecodes();
	long tableSize = sizeof(OMR::JitBuilder::BytecodeBuilder *) * numOfBytecodes;
	bytecodeBuilderTable = (OMR::JitBuilder::BytecodeBuilder **)malloc(tableSize);
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
SOMppMethod::doDup(OMR::JitBuilder::BytecodeBuilder *builder)
{
	push(builder, peek(builder));
	currentStackDepth += 1;
}

void
SOMppMethod::doPushLocal(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex)
{
	uint8_t index = method->GetBytecode(bytecodeIndex + 1);
	uint8_t level = method->GetBytecode(bytecodeIndex + 2);
	const char *contextName = getContext(builder, level);

	OMR::JitBuilder::IlValue *local =
	builder->LoadAt(pInt64,
	builder->	IndexAt(pInt64,
	builder->		LoadIndirect("VMFrame", "locals",
	builder->			Load(contextName)),
	builder->		ConstInt64(index)));

	push(builder, local);

	currentStackDepth += 1;
}

void
SOMppMethod::doPushArgument(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex)
{
	uint8_t index = method->GetBytecode(bytecodeIndex + 1);
	uint8_t level = method->GetBytecode(bytecodeIndex + 2);
	const char *contextName = getContext(builder, level);

	OMR::JitBuilder::IlValue *argument =
	builder->LoadAt(pInt64,
	builder->	IndexAt(pInt64,
	builder->		LoadIndirect("VMFrame", "arguments",
	builder->			Load(contextName)),
	builder->		ConstInt64(index)));

	push(builder, argument);

	currentStackDepth += 1;
}

void
SOMppMethod::doPushField(OMR::JitBuilder::BytecodeBuilder *builder, VMMethod *currentMethod, long bytecodeIndex)
{
	uint8_t fieldIndex = method->GetBytecode(bytecodeIndex + 1);

	OMR::JitBuilder::IlValue *outerContext = getOuterContext(builder);
	OMR::JitBuilder::IlValue *object = getSelfFromContext(builder, outerContext);
	OMR::JitBuilder::IlValue *field = nullptr;

	if (fieldIndex < FIELDNAMES_LENGTH) {
		const char *fieldName = fieldNames[fieldIndex];
		field =
		builder->LoadIndirect("VMObject", fieldName, object);
	} else {
		field =
		builder->Call("getFieldFrom", 2, object,
		builder->	ConstInt64((int64_t)fieldIndex));
	}

	push(builder, field);

	currentStackDepth += 1;
}

void
SOMppMethod::doPushBlock(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex)
{
	/* TODO come back and handle optimization in Interpreter::doPushBlock */
	VMMethod* blockMethod = static_cast<VMMethod*>(method->GetConstant(bytecodeIndex));
	long numOfArgs = blockMethod->GetNumberOfArguments();

	OMR::JitBuilder::IlValue *block =
	builder->Call("getNewBlock", 3,
	builder->	Load("frame"),
	builder->	ConstInt64((int64_t)blockMethod),
	builder->	ConstInt64((int64_t)numOfArgs));

	push(builder, block);

	currentStackDepth += 1;
}

void
SOMppMethod::doPushConstant(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex)
{
	uint8_t valueOffset = method->GetBytecode(bytecodeIndex + 1);

	OMR::JitBuilder::IlValue *constant =
	builder->LoadAt(pInt64,
	builder->	IndexAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			ConstInt64((int64_t)method->indexableFields)),
	builder->		ConstInt64(valueOffset)));

	push(builder, constant);

	currentStackDepth += 1;
}

void
SOMppMethod::doPushGlobal(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex)
{
	/* TODO If objects can move this is a runtime and not compile time fetch */
	VMSymbol* globalName = static_cast<VMSymbol*>(method->GetConstant(bytecodeIndex));

	OMR::JitBuilder::IlValue *global =
	builder->	Call("getGlobal", 1,
	builder->		ConstInt64((int64_t)globalName));

	OMR::JitBuilder::IlBuilder *globalIsNullPtr = NULL;
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

	push(builder, global);

	currentStackDepth += 1;
}

void
SOMppMethod::doPop(OMR::JitBuilder::BytecodeBuilder *builder)
{
	pop(builder);
	currentStackDepth -= 1;
}

void
SOMppMethod::doPopLocal(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex)
{
	uint8_t index = method->GetBytecode(bytecodeIndex + 1);
	uint8_t level = method->GetBytecode(bytecodeIndex + 2);

	OMR::JitBuilder::IlValue *value = peek(builder);
	pop(builder);

	const char* contextName = getContext(builder, level);

	builder->StoreAt(
	builder->	IndexAt(pInt64,
	builder->		LoadIndirect("VMFrame", "locals",
	builder->			Load(contextName)),
	builder->		ConstInt64(index)), value);

	currentStackDepth -= 1;
}

void
SOMppMethod::doPopArgument(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex)
{
	uint8_t index = method->GetBytecode(bytecodeIndex + 1);
	/* see Interpreter::doPopArgument and VMFrame::SetArgument.
	 * level does not appear to be used.
	uint8_t level = method->GetBytecode(bytecodeIndex + 2);
	*/

	OMR::JitBuilder::IlValue *value = peek(builder);
	pop(builder);

	/*
	 * See comment above about level not being used
	builder->Store("context",
	builder->	Load("frame"));

	if (level > 0) {
		OMR::JitBuilder::IlBuilder *iloop = NULL;
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
SOMppMethod::doPopField(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex)
{
	uint8_t fieldIndex = method->GetBytecode(bytecodeIndex + 1);

	OMR::JitBuilder::IlValue *outerContext = getOuterContext(builder);
	OMR::JitBuilder::IlValue *object = getSelfFromContext(builder, outerContext);
	OMR::JitBuilder::IlValue *value = peek(builder);
	pop(builder);

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
SOMppMethod::doSend(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex)
{
	VMSymbol* signature = static_cast<VMSymbol*>(method->GetConstant(bytecodeIndex));

	int numOfArgs = getReceiverForSend(builder, signature);

	/* Check for inline now that I have receiver and receiverClass */
	/* They are needed for both the generic send and inline path */
	OMR::JitBuilder::IlBuilder *sendBuilder = doInlineIfPossible(builder, signature, bytecodeIndex);

	/* NULL means that the send was inlined and there is no failure case so no generic handling*/
	if (NULL != sendBuilder) {
		sendBuilder->Store("invokable",
		sendBuilder->	Call("getInvokable", 2,
		sendBuilder->		Load("receiverClass"),
		sendBuilder->		ConstInt64((int64_t)signature)));

		sendBuilder->Store("return",
		sendBuilder->	Call("doSendIfRequired", 4,
		sendBuilder->		Load("interpreter"),
		sendBuilder->		Load("frame"),
		sendBuilder->		Load("invokable"),
		sendBuilder->		ConstInt64((int64_t)bytecodeIndex)));

		OMR::JitBuilder::IlBuilder *bail = NULL;
		sendBuilder->IfThen(&bail,
		sendBuilder->	EqualTo(
		sendBuilder->		Load("return"),
		sendBuilder->		ConstInt64(-1)));

		justReturn(bail);

		OMR::JitBuilder::IlBuilder *start = (OMR::JitBuilder::IlBuilder *)bytecodeBuilderTable[0];
		sendBuilder->IfCmpNotEqual(&start,
		sendBuilder->	Load("return"),
		sendBuilder->	ConstInt64((int64_t)bytecodeIndex));
	}

	currentStackDepth -= (numOfArgs - 1);
}

void
SOMppMethod::doSuperSend(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex)
{
	VMSymbol* signature = static_cast<VMSymbol*>(method->GetConstant(bytecodeIndex));
	int numOfArgs = Signature::GetNumberOfArguments(signature);

#if SOM_METHOD_DEBUG
	char *signatureChars = signature->GetChars();
	printf(" doSuperSend to %s with inlining %d ", signatureChars, (int)doInlining);
#endif

	builder->Store("return",
	builder->	Call("doSuperSendHelper", 4,
	builder->		Load("interpreter"),
	builder->		Load("frame"),
	builder->		ConstInt64((int64_t)signature),
	builder->		ConstInt64((int64_t)bytecodeIndex)));

	OMR::JitBuilder::IlBuilder *bail = NULL;
	builder->IfThen(&bail,
	builder->   EqualTo(
	builder->		Load("return"),
	builder->		ConstInt64(-1)));

	justReturn(bail);

	OMR::JitBuilder::BytecodeBuilder *start = bytecodeBuilderTable[0];
	
	builder->IfCmpNotEqual(&start,
	builder->	Load("return"),
	builder->	ConstInt64((int64_t)bytecodeIndex));

	currentStackDepth -= (numOfArgs - 1);
}

void
SOMppMethod::doReturnLocal(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex)
{
	OMR::JitBuilder::IlValue *result = peek(builder);

//	builder->StoreIndirect("VMFrame", "stack_ptr",
//	builder->	Load("frame"),
//	builder->	ConvertTo(pInt64,
//	builder->		Sub(
//	builder->			LoadIndirect("VMFrame", "stack_ptr",
//	builder->				Load("frame")),
//	builder->			ConstInt64(8))));

	pop(builder);

	stack->Commit(builder);

	builder->Call("popFrameAndPushResult", 3,
	builder->	Load("interpreter"),
	builder->	Load("frame"), result);

	justReturn(builder);
	/* do not adjust currentStackDepth */
}

void
SOMppMethod::doReturnNonLocal(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex)
{
	OMR::JitBuilder::IlValue *result = peek(builder);

	builder->StoreIndirect("VMFrame", "stack_ptr",
	builder->	Load("frame"),
	builder->	ConvertTo(pInt64,
	builder->		Sub(
	builder->			LoadIndirect("VMFrame", "stack_ptr",
	builder->				Load("frame")),
	builder->			ConstInt64(8))));

	builder->Store("return",
	builder->	Call("popToContext", 2,
	builder->		Load("interpreter"),
	builder->		Load("frame")));

	OMR::JitBuilder::IlBuilder *continuePath = NULL;
	OMR::JitBuilder::IlBuilder *didEscapedSend = NULL;

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
SOMppMethod::doJumpIfFalse(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex)
{
	OMR::JitBuilder::IlValue *value = peek(builder);
	pop(builder);

	OMR::JitBuilder::BytecodeBuilder *destBuilder = bytecodeBuilderTable[calculateBytecodeIndexForJump(bytecodeIndex)];
	builder->AddSuccessorBuilder(&destBuilder);

	builder->IfCmpEqual(&destBuilder, value,
	builder->	ConstInt64((int64_t)falseObject));

	currentStackDepth -= 1;
}

void
SOMppMethod::doJumpIfTrue(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex)
{
	OMR::JitBuilder::IlValue *value = peek(builder);
	pop(builder);

#if SOM_METHOD_DEBUG
	printf(" jump to %lu ", calculateBytecodeIndexForJump(bytecodeIndex));
#endif

	OMR::JitBuilder::BytecodeBuilder *destBuilder = bytecodeBuilderTable[calculateBytecodeIndexForJump(bytecodeIndex)];
	builder->AddSuccessorBuilder(&destBuilder);

	builder->IfCmpEqual(&destBuilder, value,
	builder->	ConstInt64((int64_t)trueObject));

	currentStackDepth -= 1;
}

void
SOMppMethod::doJump(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex)
{
	OMR::JitBuilder::BytecodeBuilder *destBuilder = bytecodeBuilderTable[calculateBytecodeIndexForJump(bytecodeIndex)];
	builder->AddSuccessorBuilder(&destBuilder);
	
	builder->Goto(&destBuilder);
	/* do not adjust currentStackDepth */
}

bool
SOMppMethod::generateILForBytecode(OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable, uint8_t bytecode, long bytecodeIndex)
{
	OMR::JitBuilder::BytecodeBuilder *builder = bytecodeBuilderTable[bytecodeIndex];
#ifdef SOM_METHOD_DEBUG
	printf("builder %lx ", (uint64_t)builder);
#endif

	if (NULL == builder) {
		printf("unexpected NULL BytecodeBuilder!\n");
		return false;
	}

	OMR::JitBuilder::BytecodeBuilder *nextBytecodeBuilder = nullptr;
	long nextBytecodeIndex = bytecodeIndex + Bytecode::GetBytecodeLength(bytecode);
	long numOfBytecodes = method->GetNumberOfBytecodes();
	if (nextBytecodeIndex < numOfBytecodes) {
		nextBytecodeBuilder = bytecodeBuilderTable[nextBytecodeIndex];
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
		doSend(builder, bytecodeBuilderTable, bytecodeIndex);
		builder->AddFallThroughBuilder(nextBytecodeBuilder);
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

OMR::JitBuilder::IlValue *
SOMppMethod::peek(OMR::JitBuilder::IlBuilder *builder)
{
	return stack->Top();
}

void
SOMppMethod::pop(OMR::JitBuilder::IlBuilder *builder)
{
//	builder->StoreIndirect("VMFrame", "stack_ptr",
//	builder->	Load("frame"),
//	builder->	ConvertTo(pInt64,
//	builder->		Sub(
//	builder->			LoadIndirect("VMFrame", "stack_ptr",
//	builder->				Load("frame")),
//	builder->			ConstInt64(8))));

	stack->Pop(builder);
}
void
SOMppMethod::push(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *value)
{
//	OMR::JitBuilder::IlValue *newSP =
//	builder->Add(
//	builder->	LoadIndirect("VMFrame", "stack_ptr",
//	builder->		Load("frame")),
//	builder->	ConstInt64(8));
//
//	/* write to stack */
//	builder->StoreAt(
//	builder->	ConvertTo(pInt64, newSP), value);
//
//	builder->StoreIndirect("VMFrame", "stack_ptr",
//	builder->	Load("frame"),
//	builder->	ConvertTo(pInt64, newSP));

	stack->Push(builder, value);
}

const char *
SOMppMethod::getContext(OMR::JitBuilder::IlBuilder *builder, uint8_t level)
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
			OMR::JitBuilder::IlBuilder *iloop = NULL;
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

OMR::JitBuilder::IlValue *
SOMppMethod::getOuterContext(OMR::JitBuilder::IlBuilder *builder)
{
	builder->Store("next",
	builder->	Load("frame"));

	builder->Store("contextNotNull",
	builder->	NotEqualTo(
	builder->		Load("next"),
	builder->		NullAddress()));

	OMR::JitBuilder::IlBuilder *loop = nullptr;
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

OMR::JitBuilder::IlValue *
SOMppMethod::getSelfFromContext(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *context)
{
	OMR::JitBuilder::IlValue *object =
	builder->LoadAt(pInt64,
	builder->	IndexAt(pInt64,
	builder->		LoadIndirect("VMFrame", "arguments", context),
	builder->		ConstInt64(0)));

	return object;
}

int
SOMppMethod::getReceiverForSend(OMR::JitBuilder::IlBuilder *builder, VMSymbol* signature)
{
	int numOfArgs = Signature::GetNumberOfArguments(signature);

	builder->Store("receiverAddress",
	builder->	Add(
	builder->		LoadIndirect("VMFrame", "stack_ptr",
	builder->			Load("frame")),
	builder->		ConstInt64(((int64_t)numOfArgs - 1) * -8)));

	builder->Store("receiverObject",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			Load("receiverAddress"))));

	builder->Store("receiverClass",
	builder->	Call("getClass", 1,
	builder->		Load("receiverObject")));

	return numOfArgs;
}

OMR::JitBuilder::IlValue *
SOMppMethod::getNumberOfIndexableFields(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *array)
{
	OMR::JitBuilder::IlValue *objectSize = builder->LoadIndirect("VMObject", "objectSize", array);
	OMR::JitBuilder::IlValue *numberOfFields = builder->LoadIndirect("VMObject", "numberOfFields", array);

	OMR::JitBuilder::IlValue *extraSpace =
	builder->Sub(objectSize,
	builder->	Add(
	builder->		ConstInt64(sizeof(VMObject)),
	builder->		Mul(
	builder->			ConstInt64(sizeof(VMObject*)), numberOfFields)));

	OMR::JitBuilder::IlValue *numberOfIndexableFields =
	builder->Div(extraSpace,
	builder->	ConstInt64(sizeof(VMObject*)));

	return numberOfIndexableFields;
}

void
SOMppMethod::getIndexableFieldSlot(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *array)
{
	OMR::JitBuilder::IlValue *numberOfFields = builder->LoadIndirect("VMObject", "numberOfFields", array);
	OMR::JitBuilder::IlValue *vmObjectSize = builder->ConstInt64(sizeof(VMObject));
	OMR::JitBuilder::IlValue *vmObjectPointerSize = builder->ConstInt64(sizeof(VMObject*));
	OMR::JitBuilder::IlValue *index =
	builder->Add(
	builder->	Load("indexValue"), numberOfFields);

	OMR::JitBuilder::IlValue *actualIndex =
	builder->Sub(index,
	builder->	ConstInt64(1));

	OMR::JitBuilder::IlValue *offset =
	builder->Add(vmObjectSize,
	builder->	Mul(actualIndex, vmObjectPointerSize));

	builder->Store("indexableFieldSlot",
	builder->	Add(array, offset));
}

/*************************************************
 * INLINE HELPERS
 *************************************************/

OMR::JitBuilder::IlBuilder *
SOMppMethod::verifyIntegerObject(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *object, OMR::JitBuilder::IlBuilder **failPath)
{
	OMR::JitBuilder::IlBuilder *isIntegerPath = nullptr;

#if USE_TAGGING
	builder->IfThenElse(&isIntegerPath, failPath,
	builder->	And(object,
	builder->		ConstInt64(0x1)));
#else
	builder->Store("objectClass",
	builder->	Call("getClass", 1, object));

	builder->IfThenElse(&isIntegerPath, failPath,
	builder->	EqualTo(
	builder->		Load("objectClass"),
	builder->		ConstInt64((int64_t)integerClass)));
#endif

	return isIntegerPath;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::getIntegerValue(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *object, const char *valueName, OMR::JitBuilder::IlBuilder **failPath)
{
	OMR::JitBuilder::IlBuilder *isIntegerPath = verifyIntegerObject(builder, object, failPath);

#if USE_TAGGING
	isIntegerPath->Store(valueName,
	isIntegerPath->	ShiftR(object,
	isIntegerPath->	ConstInt64(1)));
#else
	/* Read embedded slot vtable slot + gcField */
	isIntegerPath->Store("objectValueSlot",
	isIntegerPath->	Add(object,
	isIntegerPath->		ConstInt64((int64_t)(sizeof(int64_t)+sizeof(size_t)))));

	/* Read value */
	isIntegerPath->Store(valueName,
	isIntegerPath->	LoadAt(pInt64,
	isIntegerPath->		ConvertTo(pInt64,
	isIntegerPath->			Load("objectValueSlot"))));
#endif

	return isIntegerPath;
}

void
SOMppMethod::createNewInteger(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *integerValue)
{
#if USE_TAGGING
	OMR::JitBuilder::IlBuilder *lessThanMin = nullptr;
	OMR::JitBuilder::IlBuilder *possibleForTagging = nullptr;
	builder->IfThenElse(&lessThanMin, &possibleForTagging,
	builder->	GreaterThan(
	builder->		ConstInt64((int64_t)VMTAGGEDINTEGER_MIN), integerValue));

	lessThanMin->Store("newInteger",
	lessThanMin->	Call("newInteger", 1, integerValue));

	OMR::JitBuilder::IlBuilder *greaterThanMax = nullptr;
	OMR::JitBuilder::IlBuilder *tag = nullptr;
	possibleForTagging->IfThenElse(&greaterThanMax, &tag,
	possibleForTagging->	LessThan(
	possibleForTagging->		ConstInt64((int64_t)VMTAGGEDINTEGER_MAX), integerValue));

	greaterThanMax->Store("newInteger",
	greaterThanMax->	Call("newInteger", 1, integerValue));

	tag->Store("shiftedValue",
	tag->	ShiftL(integerValue,
	tag->		ConstInt64(1)));
	tag->Store("newInteger",
	tag->	Add(
	tag->		Load("shiftedValue"),
	tag->		ConstInt64(1)));
#else
	builder->Store("newInteger",
	builder->	Call("newInteger", 1, integerValue));
#endif
}

/**
 * generates IL for integer operations. The builder returned will have
 * verified that the receiver and parameter are both Integers and popped the
 * parameter off of the stack. It will then fetch rightValueInteger and
 * leftValueInteger from these objects.  If either are not Integers then failPath
 * will have sp at the original state.
 */
OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForIntergerOps(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlBuilder **failPath)
{
	OMR::JitBuilder::IlValue *sp =
	builder->LoadIndirect("VMFrame", "stack_ptr",
	builder->	Load("frame"));

	OMR::JitBuilder::IlValue *rightObject =
	builder->LoadAt(pInt64,
	builder->	ConvertTo(pInt64, sp));

	OMR::JitBuilder::IlBuilder *successPath = getIntegerValue(builder, rightObject, "rightValueInteger", failPath);

	successPath->Store("spForIntegerOps",
	successPath->	Sub(sp,
	successPath->		ConstInt64(8)));

	OMR::JitBuilder::IlValue *leftObject =
	successPath->LoadAt(pInt64,
	successPath->	ConvertTo(pInt64,
	successPath->		Load("spForIntegerOps")));

	successPath = getIntegerValue(successPath, leftObject, "leftValueInteger", failPath);

	/* set the stack up so it is at the right place to store the result.
	 * Effectively popping the second parameter and leaving stack_ptr at
	 * the first parameter so we can just store the result there */
	successPath->StoreIndirect("VMFrame", "stack_ptr",
	successPath->	Load("frame"),
	successPath->	ConvertTo(pInt64,
	successPath->		Load("spForIntegerOps")));

	return successPath;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForIntegerLessThan(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *failInline = nullptr;
	OMR::JitBuilder::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	OMR::JitBuilder::IlBuilder *thenPath = nullptr;
	OMR::JitBuilder::IlBuilder *elsePath = nullptr;
	isIntegerPath->IfThenElse(&thenPath, &elsePath,
	isIntegerPath->	LessThan(
	isIntegerPath->		Load("leftValueInteger"),
	isIntegerPath->		Load("rightValueInteger")));

	/* store true result */
	thenPath->StoreAt(
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForIntegerOps")),
	thenPath->	ConstInt64((int64_t)trueObject));

	/* store false result */
	elsePath->StoreAt(
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("spForIntegerOps")),
	elsePath->	ConstInt64((int64_t)falseObject));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForIntegerLessThanEqual(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *failInline = nullptr;
	OMR::JitBuilder::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	OMR::JitBuilder::IlBuilder *thenPath = nullptr;
	OMR::JitBuilder::IlBuilder *elsePath = nullptr;
	isIntegerPath->IfThenElse(&thenPath, &elsePath,
	isIntegerPath->	GreaterThan(
	isIntegerPath->		Load("leftValueInteger"),
	isIntegerPath->		Load("rightValueInteger")));

	/* store true result */
	thenPath->StoreAt(
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForIntegerOps")),
	thenPath->	ConstInt64((int64_t)falseObject));

	/* store false result */
	elsePath->StoreAt(
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("spForIntegerOps")),
	elsePath->	ConstInt64((int64_t)trueObject));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForIntegerGreaterThan(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *failInline = nullptr;
	OMR::JitBuilder::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	OMR::JitBuilder::IlBuilder *thenPath = nullptr;
	OMR::JitBuilder::IlBuilder *elsePath = nullptr;
	isIntegerPath->IfThenElse(&thenPath, &elsePath,
	isIntegerPath->	GreaterThan(
	isIntegerPath->		Load("leftValueInteger"),
	isIntegerPath->		Load("rightValueInteger")));

	/* store true result */
	thenPath->StoreAt(
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForIntegerOps")),
	thenPath->	ConstInt64((int64_t)trueObject));

	/* store false result */
	elsePath->StoreAt(
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("spForIntegerOps")),
	elsePath->	ConstInt64((int64_t)falseObject));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForIntegerGreaterThanEqual(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *failInline = nullptr;
	OMR::JitBuilder::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	OMR::JitBuilder::IlBuilder *thenPath = nullptr;
	OMR::JitBuilder::IlBuilder *elsePath = nullptr;
	isIntegerPath->IfThenElse(&thenPath, &elsePath,
	isIntegerPath->	LessThan(
	isIntegerPath->		Load("leftValueInteger"),
	isIntegerPath->		Load("rightValueInteger")));

	/* store true result */
	thenPath->StoreAt(
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForIntegerOps")),
	thenPath->	ConstInt64((int64_t)falseObject));

	/* store false result */
	elsePath->StoreAt(
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("spForIntegerOps")),
	elsePath->	ConstInt64((int64_t)trueObject));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForIntegerEqual(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *failInline = nullptr;
	OMR::JitBuilder::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	OMR::JitBuilder::IlBuilder *thenPath = nullptr;
	OMR::JitBuilder::IlBuilder *elsePath = nullptr;
	isIntegerPath->IfThenElse(&thenPath, &elsePath,
	isIntegerPath->	EqualTo(
	isIntegerPath->		Load("leftValueInteger"),
	isIntegerPath->		Load("rightValueInteger")));

	/* store true result */
	thenPath->StoreAt(
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForIntegerOps")),
	thenPath->	ConstInt64((int64_t)trueObject));

	/* store false result */
	elsePath->StoreAt(
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("spForIntegerOps")),
	elsePath->	ConstInt64((int64_t)falseObject));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForIntegerNotEqual(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *failInline = nullptr;
	OMR::JitBuilder::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	OMR::JitBuilder::IlBuilder *thenPath = nullptr;
	OMR::JitBuilder::IlBuilder *elsePath = nullptr;
	isIntegerPath->IfThenElse(&thenPath, &elsePath,
	isIntegerPath->	NotEqualTo(
	isIntegerPath->		Load("leftValueInteger"),
	isIntegerPath->		Load("rightValueInteger")));

	/* store true result */
	thenPath->StoreAt(
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForIntegerOps")),
	thenPath->	ConstInt64((int64_t)trueObject));

	/* store false result */
	elsePath->StoreAt(
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("spForIntegerOps")),
	elsePath->	ConstInt64((int64_t)falseObject));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForIntegerPlus(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *failInline = nullptr;
	OMR::JitBuilder::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	OMR::JitBuilder::IlValue *integerValue =
	isIntegerPath->Add(
	isIntegerPath->	Load("leftValueInteger"),
	isIntegerPath->	Load("rightValueInteger"));

	createNewInteger(isIntegerPath, integerValue);

	/* store new integer */
	isIntegerPath->StoreAt(
	isIntegerPath->	ConvertTo(pInt64,
	isIntegerPath->		Load("spForIntegerOps")),
	isIntegerPath->	Load("newInteger"));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForIntegerMinus(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *failInline = nullptr;
	OMR::JitBuilder::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	OMR::JitBuilder::IlValue *integerValue =
	isIntegerPath->Sub(
	isIntegerPath->	Load("leftValueInteger"),
	isIntegerPath->	Load("rightValueInteger"));

	createNewInteger(isIntegerPath, integerValue);

	/* store new integer */
	isIntegerPath->StoreAt(
	isIntegerPath->	ConvertTo(pInt64,
	isIntegerPath->		Load("spForIntegerOps")),
	isIntegerPath->	Load("newInteger"));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForIntegerStar(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *failInline = nullptr;
	OMR::JitBuilder::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	OMR::JitBuilder::IlValue *integerValue =
	isIntegerPath->Mul(
	isIntegerPath->	Load("leftValueInteger"),
	isIntegerPath->	Load("rightValueInteger"));

	createNewInteger(isIntegerPath, integerValue);

	/* store new integer */
	isIntegerPath->StoreAt(
	isIntegerPath->	ConvertTo(pInt64,
	isIntegerPath->		Load("spForIntegerOps")),
	isIntegerPath->	Load("newInteger"));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForIntegerValue(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *failInline = nullptr;
	OMR::JitBuilder::IlValue *sp =
	builder->LoadIndirect("VMFrame", "stack_ptr",
	builder->	Load("frame"));

	OMR::JitBuilder::IlValue *currentObject =
	builder->LoadAt(pInt64,
	builder->	ConvertTo(pInt64, sp));

	/* If it is an integer there is nothing to do */
	verifyIntegerObject(builder, currentObject, &failInline);

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForIntegerMax(OMR::JitBuilder::BytecodeBuilder *builder)
{
	/* TODO fix because rightObject does not exist */
	OMR::JitBuilder::IlBuilder *failInline = nullptr;
	OMR::JitBuilder::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	OMR::JitBuilder::IlBuilder *thenPath = nullptr;
	isIntegerPath->IfThen(&thenPath,
	isIntegerPath->	LessThan(
	isIntegerPath->		Load("leftValueInteger"),
	isIntegerPath->		Load("rightValueInteger")));

	/* store rightObject as it is the max. If leftObject (receiver) is the max value
	 * there is nothing to do since it is already the value at the right location on
	 * the stack */
	thenPath->StoreAt(
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForIntegerOps")),
	thenPath->	Load("rightObject"));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForIntegerNegated(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlValue *sp =
	builder->LoadIndirect("VMFrame", "stack_ptr",
	builder->	Load("frame"));

	OMR::JitBuilder::IlValue *integerObject =
	builder->LoadAt(pInt64,
	builder->	ConvertTo(pInt64, sp));

	OMR::JitBuilder::IlBuilder *failInline = nullptr;
	OMR::JitBuilder::IlBuilder *isIntegerPath = getIntegerValue(builder, integerObject, "integerValue", &failInline);

	OMR::JitBuilder::IlValue *integerValue =
	isIntegerPath->Sub(
	isIntegerPath->	ConstInt64(0),
	isIntegerPath->	Load("integerValue"));

	createNewInteger(isIntegerPath, integerValue);

	/* store new integer */
	isIntegerPath->StoreAt(
	isIntegerPath->	ConvertTo(pInt64, sp),
	isIntegerPath->	Load("newInteger"));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForIntegerAbs(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlValue *sp =
	builder->LoadIndirect("VMFrame", "stack_ptr",
	builder->	Load("frame"));

	OMR::JitBuilder::IlValue *integerObject =
	builder->LoadAt(pInt64,
	builder->	ConvertTo(pInt64, sp));

	OMR::JitBuilder::IlBuilder *failInline = nullptr;
	OMR::JitBuilder::IlBuilder *isIntegerPath = getIntegerValue(builder, integerObject, "integerValue", &failInline);

	OMR::JitBuilder::IlBuilder *isNegative = nullptr;
	isIntegerPath->IfThen(&isNegative,
	isIntegerPath->	LessThan(
	isIntegerPath->		Load("integerValue"),
	isIntegerPath->		ConstInt64(0)));

	OMR::JitBuilder::IlValue *integerValue =
	isNegative->Sub(
	isNegative->	ConstInt64(0),
	isNegative->	Load("integerValue"));

	createNewInteger(isNegative, integerValue);

	/* store new integer */
	isNegative->StoreAt(
	isNegative->	ConvertTo(pInt64, sp),
	isNegative->	Load("newInteger"));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForArrayAt(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *failInline = nullptr;

	OMR::JitBuilder::IlValue *sp =
	builder->LoadIndirect("VMFrame", "stack_ptr",
	builder->	Load("frame"));

	OMR::JitBuilder::IlValue *indexObject =
	builder->LoadAt(pInt64,
	builder->	ConvertTo(pInt64, sp));

	OMR::JitBuilder::IlBuilder *indexIsInteger = getIntegerValue(builder, indexObject, "indexValue", &failInline);

	OMR::JitBuilder::IlValue * spForReceiver =
	indexIsInteger->Sub(sp,
	indexIsInteger->	ConstInt64(8));

	OMR::JitBuilder::IlValue *arrayObject =
	indexIsInteger->LoadAt(pInt64,
	indexIsInteger->	ConvertTo(pInt64, spForReceiver));

	indexIsInteger->Store("arrayClass",
	indexIsInteger->	LoadIndirect("VMObject", "clazz", arrayObject));

	OMR::JitBuilder::IlBuilder *isArrayPath = nullptr;
	indexIsInteger->IfThenElse(&isArrayPath, &failInline,
	indexIsInteger->	EqualTo(
	indexIsInteger->		Load("arrayClass"),
	indexIsInteger->		ConstInt64((int64_t)arrayClass)));

	getIndexableFieldSlot(isArrayPath, arrayObject);

	/* Read value */
	isArrayPath->Store("result",
	isArrayPath->	LoadAt(pInt64,
	isArrayPath->		ConvertTo(pInt64,
	isArrayPath->			Load("indexableFieldSlot"))));

	/* decrement the stack to hold result */
	/* this method had 2 args on the stack so it only has to pop 1 arg to store result */
	isArrayPath->StoreIndirect("VMFrame", "stack_ptr",
	isArrayPath->	Load("frame"),
	isArrayPath->	ConvertTo(pInt64, spForReceiver));

	/* store new integer */
	isArrayPath->StoreAt(
	isArrayPath->	ConvertTo(pInt64, spForReceiver),
	isArrayPath->	Load("result"));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForArrayAtPut(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *failInline = nullptr;

	OMR::JitBuilder::IlValue *sp =
	builder->LoadIndirect("VMFrame", "stack_ptr",
	builder->	Load("frame"));

	builder->Store("valueObject",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64, sp)));

	OMR::JitBuilder::IlValue *indexObject =
	builder->LoadAt(pInt64,
	builder->	ConvertTo(pInt64,
	builder->		Sub(sp,
	builder->			ConstInt64(8))));

	OMR::JitBuilder::IlBuilder *indexIsInteger = getIntegerValue(builder, indexObject, "indexValue", &failInline);

	OMR::JitBuilder::IlValue * spForReceiver =
	indexIsInteger->Sub(sp,
	indexIsInteger->	ConstInt64(16));

	/* peek the array object without popping */
	OMR::JitBuilder::IlValue *arrayObject =
	indexIsInteger->LoadAt(pInt64,
	indexIsInteger->	ConvertTo(pInt64, spForReceiver));

	indexIsInteger->Store("arrayClass",
	indexIsInteger->	LoadIndirect("VMObject", "clazz", arrayObject));

	OMR::JitBuilder::IlBuilder *isArrayPath = nullptr;
	indexIsInteger->IfThenElse(&isArrayPath, &failInline,
	indexIsInteger->	EqualTo(
	indexIsInteger->		Load("arrayClass"),
	indexIsInteger->		ConstInt64((int64_t)arrayClass)));

	getIndexableFieldSlot(isArrayPath, arrayObject);

	/* This function needs to pop 2 params but leave the receiver on the stack */
	isArrayPath->StoreIndirect("VMFrame", "stack_ptr",
	isArrayPath->	Load("frame"),
	isArrayPath->	ConvertTo(pInt64, spForReceiver));

	/* set indexable field to valueObject */
	isArrayPath->StoreAt(
	isArrayPath->	ConvertTo(pInt64,
	isArrayPath->		Load("indexableFieldSlot")),
	isArrayPath->	Load("valueObject"));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForArrayLength(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *failInline = nullptr;

	OMR::JitBuilder::IlValue *sp =
	builder->LoadIndirect("VMFrame", "stack_ptr",
	builder->	Load("frame"));

	OMR::JitBuilder::IlValue *arrayObject =
	builder->LoadAt(pInt64,
	builder->	ConvertTo(pInt64, sp));

	builder->Store("arrayClass",
	builder->	Call("getClass", 1, arrayObject));

	OMR::JitBuilder::IlBuilder *isArrayPath = nullptr;

	builder->IfThenElse(&isArrayPath, &failInline,
	builder->	EqualTo(
	builder->		Load("arrayClass"),
	builder->		ConstInt64((int64_t)arrayClass)));

	createNewInteger(isArrayPath, getNumberOfIndexableFields(isArrayPath, arrayObject));

	/* store new integer */
	isArrayPath->StoreAt(
	isArrayPath->	ConvertTo(pInt64, sp),
	isArrayPath->	Load("newInteger"));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForNilisNil(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *isNil = NULL;
	OMR::JitBuilder::IlBuilder *notNil = NULL;

	OMR::JitBuilder::IlValue *sp =
	builder->LoadIndirect("VMFrame", "stack_ptr",
	builder->	Load("frame"));

	builder->Store("currentObject",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64, sp)));

	builder->Store("nilObject",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			ConstInt64((int64_t)&nilObject))));

	builder->IfThenElse(&isNil, &notNil,
	builder->	EqualTo(
	builder->		Load("currentObject"),
	builder->		Load("nilObject")));

	isNil->StoreAt(
	isNil->	ConvertTo(pInt64, sp),
	isNil-> LoadAt(pInt64,
	isNil->		ConvertTo(pInt64,
	isNil->			ConstInt64((int64_t)&trueObject))));

	notNil->StoreAt(
	notNil->	ConvertTo(pInt64, sp),
	notNil-> LoadAt(pInt64,
	notNil->	ConvertTo(pInt64,
	notNil->		ConstInt64((int64_t)&falseObject))));

	return nullptr;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForBooleanNot(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *isTrue = NULL;
	OMR::JitBuilder::IlBuilder *notTrue = NULL;

	OMR::JitBuilder::IlValue *sp =
	builder->LoadIndirect("VMFrame", "stack_ptr",
	builder->	Load("frame"));

	builder->Store("currentObject",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64, sp)));

	builder->Store("trueObject",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			ConstInt64((int64_t)&trueObject))));

	builder->Store("falseObject",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			ConstInt64((int64_t)&falseObject))));

	builder->IfThenElse(&isTrue, &notTrue,
	builder->	EqualTo(
	builder->		Load("currentObject"),
	builder->		Load("trueObject")));

	isTrue->StoreAt(
	isTrue->	ConvertTo(pInt64, sp),
	isTrue->	Load("falseObject"));

	OMR::JitBuilder::IlBuilder *isFalse = NULL;
	OMR::JitBuilder::IlBuilder *failInline = NULL;

	notTrue->IfThenElse(&isFalse, &failInline,
	notTrue->	EqualTo(
	notTrue->		Load("currentObject"),
	notTrue->		Load("falseObject")));

	isFalse->StoreAt(
	isFalse->	ConvertTo(pInt64, sp),
	isFalse->	Load("trueObject"));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForDoubleOps(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlBuilder **failPath)
{
	OMR::JitBuilder::IlValue *sp =
	builder->LoadIndirect("VMFrame", "stack_ptr",
	builder->	Load("frame"));

	builder->Store("rightObjectDouble",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64, sp)));

	builder->Store("rightClassDouble",
	builder->	Call("getClass", 1,
	builder->		Load("rightObjectDouble")));

	OMR::JitBuilder::IlBuilder *rightIsDouble = NULL;
	builder->IfThenElse(&rightIsDouble, failPath,
	builder->	EqualTo(
	builder->		Load("rightClassDouble"),
	builder->		ConstInt64((int64_t)doubleClass)));

	rightIsDouble->Store("spForDoubleOps",
	rightIsDouble->	Sub(sp,
	rightIsDouble->		ConstInt64(8)));

	rightIsDouble->Store("leftObjectDouble",
	rightIsDouble->	LoadAt(pInt64,
	rightIsDouble->		ConvertTo(pInt64,
	rightIsDouble->			Load("spForDoubleOps"))));

	rightIsDouble->Store("leftClassDouble",
	rightIsDouble->	Call("getClass", 1,
	rightIsDouble->		Load("leftObjectDouble")));

	OMR::JitBuilder::IlBuilder *isDoublePath = NULL;
	rightIsDouble->IfThenElse(&isDoublePath, failPath,
	rightIsDouble->	EqualTo(
	rightIsDouble->		Load("leftClassDouble"),
	rightIsDouble->		ConstInt64((int64_t)doubleClass)));

	/* set the stack up so it is at the right place to store the result.
	 * Effectively popping the second parameter and leaving stack_ptr at
	 * the first parameter so we can just store the result there */
	isDoublePath->StoreIndirect("VMFrame", "stack_ptr",
	isDoublePath->	Load("frame"),
	isDoublePath->	ConvertTo(pInt64,
	isDoublePath->		Load("spForDoubleOps")));

	/* Read right embedded slot vtable slot + gcField */
	isDoublePath->Store("rightValueSlotDouble",
	isDoublePath->	Add(
	isDoublePath->		Load("rightObjectDouble"),
	isDoublePath->		ConstInt64((int64_t)(sizeof(int64_t)+sizeof(size_t)))));

	/* Read value */
	isDoublePath->Store("rightValueDouble",
	isDoublePath->	LoadAt(pDouble,
	isDoublePath->		ConvertTo(pDouble,
	isDoublePath->			Load("rightValueSlotDouble"))));

	/* Read left embedded slot vtable slot + gcField */
	isDoublePath->Store("leftValueSlotDouble",
	isDoublePath->	Add(
	isDoublePath->		Load("leftObjectDouble"),
	isDoublePath->		ConstInt64((int64_t)(sizeof(int64_t)+sizeof(size_t)))));

	/* Read value */
	isDoublePath->Store("leftValueDouble",
	isDoublePath->	LoadAt(pDouble,
	isDoublePath->		ConvertTo(pDouble,
	isDoublePath->			Load("leftValueSlotDouble"))));

	return isDoublePath;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForDoubleLessThan(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *failInline = NULL;
	OMR::JitBuilder::IlBuilder *isDoublePath = generateILForDoubleOps(builder, &failInline);

	OMR::JitBuilder::IlBuilder *thenPath = NULL;
	OMR::JitBuilder::IlBuilder *elsePath = NULL;
	isDoublePath->IfThenElse(&thenPath, &elsePath,
	isDoublePath->	LessThan(
	isDoublePath->		Load("leftValueDouble"),
	isDoublePath->		Load("rightValueDouble")));

	/* store true result */
	thenPath->StoreAt(
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForDoubleOps")),
	thenPath->	ConstInt64((int64_t)trueObject));

	/* store false result */
	elsePath->StoreAt(
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("spForDoubleOps")),
	elsePath->	ConstInt64((int64_t)falseObject));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForDoubleLessThanEqual(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *failInline = NULL;
	OMR::JitBuilder::IlBuilder *isDoublePath = generateILForDoubleOps(builder, &failInline);

	OMR::JitBuilder::IlBuilder *thenPath = NULL;
	OMR::JitBuilder::IlBuilder *elsePath = NULL;
	isDoublePath->IfThenElse(&thenPath, &elsePath,
	isDoublePath->	GreaterThan(
	isDoublePath->		Load("leftValueDouble"),
	isDoublePath->		Load("rightValueDouble")));

	/* store true result */
	thenPath->StoreAt(
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForDoubleOps")),
	thenPath->	ConstInt64((int64_t)falseObject));

	/* store false result */
	elsePath->StoreAt(
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("spForDoubleOps")),
	elsePath->	ConstInt64((int64_t)trueObject));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForDoubleGreaterThan(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *failInline = NULL;
	OMR::JitBuilder::IlBuilder *isDoublePath = generateILForDoubleOps(builder, &failInline);

	OMR::JitBuilder::IlBuilder *thenPath = NULL;
	OMR::JitBuilder::IlBuilder *elsePath = NULL;
	isDoublePath->IfThenElse(&thenPath, &elsePath,
	isDoublePath->	GreaterThan(
	isDoublePath->		Load("leftValueDouble"),
	isDoublePath->		Load("rightValueDouble")));

	/* store true result */
	thenPath->StoreAt(
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForDoubleOps")),
	thenPath->	ConstInt64((int64_t)trueObject));

	/* store false result */
	elsePath->StoreAt(
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("spForDoubleOps")),
	elsePath->	ConstInt64((int64_t)falseObject));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForDoubleGreaterThanEqual(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *failInline = NULL;
	OMR::JitBuilder::IlBuilder *isDoublePath = generateILForDoubleOps(builder, &failInline);

	OMR::JitBuilder::IlBuilder *thenPath = NULL;
	OMR::JitBuilder::IlBuilder *elsePath = NULL;
	isDoublePath->IfThenElse(&thenPath, &elsePath,
	isDoublePath->	LessThan(
	isDoublePath->		Load("leftValueDouble"),
	isDoublePath->		Load("rightValueDouble")));

	/* store true result */
	thenPath->StoreAt(
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForDoubleOps")),
	thenPath->	ConstInt64((int64_t)falseObject));

	/* store false result */
	elsePath->StoreAt(
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("spForDoubleOps")),
	elsePath->	ConstInt64((int64_t)trueObject));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForDoubleEqual(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *failInline = NULL;
	OMR::JitBuilder::IlBuilder *isDoublePath = generateILForDoubleOps(builder, &failInline);

	OMR::JitBuilder::IlBuilder *thenPath = NULL;
	OMR::JitBuilder::IlBuilder *elsePath = NULL;
	isDoublePath->IfThenElse(&thenPath, &elsePath,
	isDoublePath->	EqualTo(
	isDoublePath->		Load("leftValueDouble"),
	isDoublePath->		Load("rightValueDouble")));

	/* store true result */
	thenPath->StoreAt(
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForDoubleOps")),
	thenPath->	ConstInt64((int64_t)trueObject));

	/* store false result */
	elsePath->StoreAt(
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("spForDoubleOps")),
	elsePath->	ConstInt64((int64_t)falseObject));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForDoubleNotEqual(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *failInline = NULL;
	OMR::JitBuilder::IlBuilder *isDoublePath = generateILForDoubleOps(builder, &failInline);

	OMR::JitBuilder::IlBuilder *thenPath = NULL;
	OMR::JitBuilder::IlBuilder *elsePath = NULL;
	isDoublePath->IfThenElse(&thenPath, &elsePath,
	isDoublePath->	EqualTo(
	isDoublePath->		Load("leftValueDouble"),
	isDoublePath->		Load("rightValueDouble")));

	/* store true result */
	thenPath->StoreAt(
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForDoubleOps")),
	thenPath->	ConstInt64((int64_t)falseObject));

	/* store false result */
	elsePath->StoreAt(
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("spForDoubleOps")),
	elsePath->	ConstInt64((int64_t)trueObject));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForDoublePlus(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *failInline = NULL;
	OMR::JitBuilder::IlBuilder *isDoublePath = generateILForDoubleOps(builder, &failInline);

	isDoublePath->StoreAt(
	isDoublePath->	ConvertTo(pInt64,
	isDoublePath->		Load("spForDoubleOps")),
	isDoublePath->	Call("newDouble", 1,
	isDoublePath->		Add(
	isDoublePath->			Load("leftValueDouble"),
	isDoublePath->			Load("rightValueDouble"))));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForDoubleMinus(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *failInline = NULL;
	OMR::JitBuilder::IlBuilder *isDoublePath = generateILForDoubleOps(builder, &failInline);

	isDoublePath->StoreAt(
	isDoublePath->	ConvertTo(pInt64,
	isDoublePath->		Load("spForDoubleOps")),
	isDoublePath->	Call("newDouble", 1,
	isDoublePath->		Sub(
	isDoublePath->			Load("leftValueDouble"),
	isDoublePath->			Load("rightValueDouble"))));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForDoubleStar(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *failInline = NULL;
	OMR::JitBuilder::IlBuilder *isDoublePath = generateILForDoubleOps(builder, &failInline);

	isDoublePath->StoreAt(
	isDoublePath->	ConvertTo(pInt64,
	isDoublePath->		Load("spForDoubleOps")),
	isDoublePath->	Call("newDouble", 1,
	isDoublePath->		Mul(
	isDoublePath->			Load("leftValueDouble"),
	isDoublePath->			Load("rightValueDouble"))));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateILForDoubleSlashSlash(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlBuilder *failInline = NULL;
	OMR::JitBuilder::IlBuilder *isDoublePath = generateILForDoubleOps(builder, &failInline);

	isDoublePath->StoreAt(
	isDoublePath->	ConvertTo(pInt64,
	isDoublePath->		Load("spForDoubleOps")),
	isDoublePath->	Call("newDouble", 1,
	isDoublePath->		Div(
	isDoublePath->			Load("leftValueDouble"),
	isDoublePath->			Load("rightValueDouble"))));

	return failInline;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::doInlineIfPossible(OMR::JitBuilder::BytecodeBuilder *builder, VMSymbol* signature, long bytecodeIndex)
{
	VMClass *receiverFromCache = method->getInvokeReceiverCache(bytecodeIndex);
	char *signatureChars = signature->GetChars();
	OMR::JitBuilder::IlBuilder *genericSend = builder;

#if SOM_METHOD_DEBUG
	bool didInline = false;
	const char* receiverClassName = (receiverFromCache == nullptr) ? "(unknown class)" : receiverFromCache->GetName()->GetChars();
#endif

	if (doInlining) {
		if (nullptr != receiverFromCache) {
			genericSend = generateRecognizedMethod(builder, receiverFromCache, signatureChars);
			/* if the builder returned is the same as the builder passed in it did not generate inlined code */
			if (genericSend == builder) {
				VMInvokable *invokable = receiverFromCache->LookupInvokable(signature);
				if (!invokable->IsPrimitive()) {
					VMMethod *methodToInline = static_cast<VMMethod*>(invokable);
					/* Attempt to inline the method being sent */
					genericSend = generateGenericInline(builder, receiverFromCache, methodToInline, signatureChars);
#if SOM_METHOD_DEBUG
					if (genericSend != builder) {
						didInline = true;
					}
#endif
				}
			} else {
#if SOM_METHOD_DEBUG
				didInline = true;
#endif
			}
		}
	}
#if SOM_METHOD_DEBUG
	printf(" call to %s>>#%s %s", receiverClassName, signatureChars, didInline ? "inlined" : "");
#endif

	return genericSend;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateRecognizedMethod(OMR::JitBuilder::BytecodeBuilder *builder, VMClass *receiverFromCache, char *signatureChars)
{
	if ((VMClass*)integerClass == receiverFromCache) {
		if (0 == strcmp("<", signatureChars)) {
			return generateILForIntegerLessThan(builder);
		} else if (0 == strcmp("<=", signatureChars)) {
			return generateILForIntegerLessThanEqual(builder);
		} else if (0 == strcmp(">", signatureChars)) {
			return generateILForIntegerGreaterThan(builder);
		} else if (0 == strcmp(">=", signatureChars)) {
			return generateILForIntegerGreaterThanEqual(builder);
		} else if (0 == strcmp("=", signatureChars)) {
			return generateILForIntegerEqual(builder);
		} else if (0 == strcmp("<>", signatureChars)) {
			return generateILForIntegerNotEqual(builder);
		} else if (0 == strcmp("~=", signatureChars)) {
			return generateILForIntegerNotEqual(builder);
		} else if (0 == strcmp("+", signatureChars)) {
			return generateILForIntegerPlus(builder);
		} else if (0 == strcmp("-", signatureChars)) {
			return generateILForIntegerMinus(builder);
		} else if (0 == strcmp("*", signatureChars)) {
			return generateILForIntegerStar(builder);
		} else if (0 == strcmp("value", signatureChars)) {
			return generateILForIntegerValue(builder);
		} else if (0 == strcmp("max:", signatureChars) && false) {
			return generateILForIntegerMax(builder);
		} else if (0 == strcmp("negated", signatureChars)){
			return generateILForIntegerNegated(builder);
		} else if (0 == strcmp("abs", signatureChars)){
			return generateILForIntegerAbs(builder);
		}
	} else if ((VMClass*)arrayClass == receiverFromCache) {
		if (0 == strcmp("at:", signatureChars)) {
			return generateILForArrayAt(builder);
		} else if (0 == strcmp("at:put:", signatureChars)) {
			return generateILForArrayAtPut(builder);
		} else if (0 == strcmp("length", signatureChars)) {
			return generateILForArrayLength(builder);
		}
	} else if ((VMClass*)nilClass == receiverFromCache) {
		if (0 == strcmp("isNil", signatureChars)) {
			return generateILForNilisNil(builder);
		}
	} else if ((((VMClass*)falseClass == receiverFromCache) || ((VMClass*)trueClass == receiverFromCache))) {
		if (0 == strcmp("not", signatureChars)) {
			return generateILForBooleanNot(builder);
		}
	} else if ((VMClass*)doubleClass == receiverFromCache) {
		if (0 == strcmp("<", signatureChars)) {
			return generateILForDoubleLessThan(builder);
		} else if (0 == strcmp("<=", signatureChars)) {
			return generateILForDoubleLessThanEqual(builder);
		} else if (0 == strcmp(">", signatureChars)) {
			return generateILForDoubleGreaterThan(builder);
		} else if (0 == strcmp(">=", signatureChars)) {
			return generateILForDoubleGreaterThanEqual(builder);
		} else if (0 == strcmp("=", signatureChars)) {
			return generateILForDoubleEqual(builder);
		} else if (0 == strcmp("<>", signatureChars)) {
			return generateILForDoubleNotEqual(builder);
		} else if (0 == strcmp("+", signatureChars)) {
			return generateILForDoublePlus(builder);
		} else if (0 == strcmp("-", signatureChars)) {
			return generateILForDoubleMinus(builder);
		} else if (0 == strcmp("*", signatureChars)) {
			return generateILForDoubleStar(builder);
		} else if (0 == strcmp("//", signatureChars)) {
			return generateILForDoubleSlashSlash(builder);
		}
	}

	return builder;
}

OMR::JitBuilder::IlBuilder *
SOMppMethod::generateGenericInline(OMR::JitBuilder::BytecodeBuilder *builder, VMClass *receiverFromCache, VMMethod *vmMethod, char *signatureChars)
{
	OMR::JitBuilder::BytecodeBuilder *initialBilder = builder;
	OMR::JitBuilder::IlBuilder *fail = nullptr;

	/* check to see if there is room on the current stack to execute the method */
	/* spaceAvailable = # of stack slots in the current method - current depth + # of arguments for the method to inline */
	long methodToInlineNumberOfArguments = vmMethod->GetNumberOfArguments();
	long spaceAvailable = method->GetMaximumNumberOfStackElements() - currentStackDepth + methodToInlineNumberOfArguments;
	if (vmMethod->GetMaximumNumberOfStackElements() <= spaceAvailable) {
		if (methodIsInlineable(vmMethod)) {
			OMR::JitBuilder::IlBuilder *inlineBuilder = nullptr;
			/* TODO if debug mode gen code to ensure stack has room at runtime */

			/* make sure the receiver class is the right one */
			initialBilder->IfThenElse(&inlineBuilder, &fail,
			initialBilder->	EqualTo(
			initialBilder->		ConstInt64((int64_t)receiverFromCache),
			initialBilder->		Load("receiverClass")));

			inlineBuilder->Store("argumentsArray",
			inlineBuilder->	CreateLocalArray((int32_t)methodToInlineNumberOfArguments, Int64));

			for (int32_t i = 0; i < (int32_t)methodToInlineNumberOfArguments; i++) {
				inlineBuilder->StoreAt(
				inlineBuilder->	IndexAt(pInt64,
				inlineBuilder->		Load("argumentsArray"),
				inlineBuilder->		ConstInt32(i)),
				inlineBuilder->	LoadAt(pInt64,
				inlineBuilder->		IndexAt(pInt64,
				inlineBuilder->			Load("receiverAddress"),
				inlineBuilder->			ConstInt32(i))));
			}

			/* set stack_ptr for the inline method */
			inlineBuilder->StoreIndirect("VMFrame", "stack_ptr",
			inlineBuilder->	Load("frame"),
			inlineBuilder->	ConvertTo(pInt64,
			inlineBuilder->		Sub(
			inlineBuilder->			Load("receiverAddress"),
			inlineBuilder->			ConstInt64(8))));

			long bytecodeCount = vmMethod->GetNumberOfBytecodes();
			long bytecodeIndex = 0;
			while (bytecodeIndex < bytecodeCount) {
				uint8_t bc = vmMethod->GetBytecode(bytecodeIndex);

				switch(bc) {
				case BC_DUP:
				{
					push(inlineBuilder, peek(inlineBuilder));
					break;
				}
				case BC_PUSH_FIELD:
				{
					uint8_t fieldIndex = vmMethod->GetBytecode(bytecodeIndex + 1);

					OMR::JitBuilder::IlValue *fieldValue =
					inlineBuilder->Call("getFieldFrom", 2,
					inlineBuilder->	Load("receiverObject"),
					inlineBuilder->	ConstInt64((int64_t)fieldIndex));

					push(inlineBuilder, fieldValue);

					break;
				}
				case BC_PUSH_CONSTANT:
				{
					uint8_t valueOffset = vmMethod->GetBytecode(bytecodeIndex + 1);

					OMR::JitBuilder::IlValue *constant =
					inlineBuilder->LoadAt(pInt64,
					inlineBuilder->	IndexAt(pInt64,
					inlineBuilder->		ConvertTo(pInt64,
					inlineBuilder->			ConstInt64((int64_t)vmMethod->indexableFields)),
					inlineBuilder->		ConstInt64(valueOffset)));

					push(inlineBuilder, constant);

					break;
				}
				case BC_PUSH_GLOBAL:
				{
					VMSymbol* globalName = static_cast<VMSymbol*>(vmMethod->GetConstant(bytecodeIndex));

					OMR::JitBuilder::IlValue *global =
					inlineBuilder->Call("getGlobal", 1,
					inlineBuilder->	ConstInt64((int64_t)globalName));

					push(inlineBuilder, global);

					break;
				}
				case BC_PUSH_ARGUMENT:
				{
					uint8_t argumentIndex = vmMethod->GetBytecode(bytecodeIndex + 1);

					OMR::JitBuilder::IlValue *argument =
					inlineBuilder->LoadAt(pInt64,
					inlineBuilder->	IndexAt(pInt64,
					inlineBuilder->		Load("argumentsArray"),
					inlineBuilder->		ConstInt32(argumentIndex)));

					push(inlineBuilder, argument);
					break;
				}
				case BC_POP:
				{
					pop(inlineBuilder);
					break;
				}
				case BC_POP_FIELD:
				{
					uint8_t fieldIndex = vmMethod->GetBytecode(bytecodeIndex + 1);

					OMR::JitBuilder::IlValue *value = peek(inlineBuilder);
					pop(inlineBuilder);

					inlineBuilder->Call("setFieldTo", 3,
					inlineBuilder->	Load("receiverObject"),
					inlineBuilder->	ConstInt64((int64_t)fieldIndex), value);

					break;
				}
				case BC_RETURN_LOCAL:
				{
					OMR::JitBuilder::IlValue *returnValue = peek(inlineBuilder);

					/* store the return value */
					inlineBuilder->StoreAt(
					inlineBuilder->	ConvertTo(pInt64,
					inlineBuilder->		Load("receiverAddress")), returnValue);

					/* set the stack_ptr back to the appropriate place */
					inlineBuilder->StoreIndirect("VMFrame", "stack_ptr",
					inlineBuilder->	Load("frame"),
					inlineBuilder->	ConvertTo(pInt64,
					inlineBuilder->		Load("receiverAddress")));

					break;
				}
				default:
				{
					/* This should not happen!!! */
					printf("Attempting to inline a method (%s>>#%s) that contains a bytecode (%d) which is not handled",
							vmMethod->GetHolder()->GetName()->GetChars(), signatureChars, bc);
					/* TODO replace with runtime assert */
					int *x = 0;
					*x = 0;
				}
				}
				bytecodeIndex += Bytecode::GetBytecodeLength(bc);
			}
			return fail;
		}
	}

	return initialBilder;
}

bool
SOMppMethod::methodIsInlineable(VMMethod *vmMethod)
{
	long bytecodeCount = vmMethod->GetNumberOfBytecodes();
	long i = 0;
	bool isInlinable = true;
	
	while ((i < bytecodeCount) && isInlinable) {
		uint8_t bc = vmMethod->GetBytecode(i);
		switch(bc) {
		case BC_DUP:
		case BC_PUSH_FIELD:
		case BC_PUSH_CONSTANT:
		case BC_PUSH_GLOBAL:
		case BC_POP:
		case BC_POP_FIELD:
		case BC_RETURN_LOCAL:
			break;
		case BC_PUSH_ARGUMENT:
		{
			uint8_t level = vmMethod->GetBytecode(i + 2);
			if (level > 0) {
				isInlinable = false;
			}
			break;
		}
		default:
			isInlinable = false;
		}
		i += Bytecode::GetBytecodeLength(bc);
	}
	return isInlinable;
}
