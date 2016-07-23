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
	types->DefineField("VMFrame", "stack_ptr", Int64);
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

#if SOM_METHOD_DEBUG
	printf("\nGenerating code for %s\n", methodName);
#endif

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
	push(builder, peek(builder));
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

	push(builder, local);

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

	push(builder, argument);

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

	push(builder, field);

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

	push(builder, block);

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

	push(builder, constant);

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
	globalIsNullPtr->StoreAt(pInt64,
	globalIsNullPtr->	ConvertTo(pInt64,
	globalIsNullPtr->		ConstInt64(0)),
	globalIsNullPtr->	ConstInt64(0));

	justReturn(globalIsNullPtr);

	push(builder, global);

	currentStackDepth += 1;
}

void
SOMppMethod::doPop(TR::BytecodeBuilder *builder)
{
	pop(builder);
	currentStackDepth -= 1;
}

void
SOMppMethod::doPopLocal(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	uint8_t index = method->GetBytecode(bytecodeIndex + 1);
	uint8_t level = method->GetBytecode(bytecodeIndex + 2);

	TR::IlValue *value = peek(builder);
	pop(builder);

	const char* contextName = getContext(builder, level);

	builder->StoreAt(pInt64,
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

	TR::IlValue *value = peek(builder);
	pop(builder);

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

	builder->StoreAt(pInt64,
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
	TR::IlValue *value = peek(builder);
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
SOMppMethod::doSend(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex)
{
	VMSymbol* signature = static_cast<VMSymbol*>(method->GetConstant(bytecodeIndex));

	int numOfArgs = getReceiverForSend(builder, signature);

	/* Check for inline now that I have receiver and receiverClass */
	/* They are needed for both the generic send and inline path */
	TR::IlBuilder *sendBuilder = doInlineIfPossible(builder, signature, bytecodeIndex);

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

		TR::IlBuilder *bail = NULL;
		sendBuilder->IfThen(&bail,
		sendBuilder->	EqualTo(
		sendBuilder->		Load("return"),
		sendBuilder->		ConstInt64(-1)));

		justReturn(bail);

		TR::IlBuilder *start = (TR::IlBuilder *)bytecodeBuilderTable[0];
		sendBuilder->IfCmpNotEqual(&start,
		sendBuilder->	Load("return"),
		sendBuilder->	ConstInt64((int64_t)bytecodeIndex));
	}

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

	TR::IlBuilder *start = (TR::IlBuilder *)bytecodeBuilderTable[0];
	builder->IfCmpNotEqual(&start,
	builder->	Load("return"),
	builder->	ConstInt64((int64_t)bytecodeIndex));

	currentStackDepth -= (numOfArgs - 1);
}

void
SOMppMethod::doReturnLocal(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	TR::IlValue *result = peek(builder);

	builder->StoreIndirect("VMFrame", "stack_ptr",
	builder->	Load("frame"),
	builder->	ConvertTo(pInt64,
	builder->		Sub(
	builder->			LoadIndirect("VMFrame", "stack_ptr",
	builder->				Load("frame")),
	builder->			ConstInt64(8))));

	builder->Call("popFrameAndPushResult", 3,
	builder->	Load("interpreter"),
	builder->	Load("frame"), result);

	justReturn(builder);
	/* do not adjust currentStackDepth */
}

void
SOMppMethod::doReturnNonLocal(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	TR::IlValue *result = peek(builder);

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

	TR::IlBuilder *continuePath = NULL;
	TR::IlBuilder *didEscapedSend = NULL;

	builder->IfThenElse(&continuePath, &didEscapedSend,
	builder->	EqualTo(
	builder->		Load("return"),
	builder->		ConstInt64(0)));

	didEscapedSend->Call("printString", 1,
	didEscapedSend->	ConstInt64((int64_t)"\n\n\n doReturnNonLocal crashing due to escapedBlock\n\n\n\n"));
	didEscapedSend->StoreAt(pInt64,
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
	TR::IlValue *value = peek(builder);
	pop(builder);

	TR::BytecodeBuilder *destBuilder = bytecodeBuilderTable[calculateBytecodeIndexForJump(bytecodeIndex)];
	builder->AddSuccessorBuilder(destBuilder);

	TR::IlBuilder *dest = (TR::IlBuilder *)destBuilder;
	builder->IfCmpEqual(&dest, value,
	builder->	ConstInt64((int64_t)falseObject));

	currentStackDepth -= 1;
}

void
SOMppMethod::doJumpIfTrue(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex)
{
	TR::IlValue *value = peek(builder);
	pop(builder);

#if SOM_METHOD_DEBUG
	printf(" jump to %lu ", calculateBytecodeIndexForJump(bytecodeIndex));
#endif

	TR::BytecodeBuilder *destBuilder = bytecodeBuilderTable[calculateBytecodeIndexForJump(bytecodeIndex)];
	builder->AddSuccessorBuilder(destBuilder);

	TR::IlBuilder *dest = (TR::IlBuilder *)destBuilder;
	builder->IfCmpEqual(&dest, value,
	builder->	ConstInt64((int64_t)trueObject));

	currentStackDepth -= 1;
}

void
SOMppMethod::doJump(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex)
{
	TR::BytecodeBuilder *destBuilder = bytecodeBuilderTable[calculateBytecodeIndexForJump(bytecodeIndex)];
	builder->AddSuccessorBuilder(destBuilder);

	TR::IlBuilder *dest = (TR::IlBuilder *)destBuilder;
	builder->Goto(&dest);
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

TR::IlValue *
SOMppMethod::peek(TR::IlBuilder *builder)
{
	TR::IlValue *value =
	builder->LoadAt(pInt64,
	builder->	ConvertTo(pInt64,
	builder->		LoadIndirect("VMFrame", "stack_ptr",
	builder->			Load("frame"))));

	return value;
}

void
SOMppMethod::pop(TR::IlBuilder *builder)
{
	builder->StoreIndirect("VMFrame", "stack_ptr",
	builder->	Load("frame"),
	builder->	ConvertTo(pInt64,
	builder->		Sub(
	builder->			LoadIndirect("VMFrame", "stack_ptr",
	builder->				Load("frame")),
	builder->			ConstInt64(8))));
}
void
SOMppMethod::push(TR::IlBuilder *builder, TR::IlValue *value)
{
	TR::IlValue *newSP =
	builder->Add(
	builder->	LoadIndirect("VMFrame", "stack_ptr",
	builder->		Load("frame")),
	builder->	ConstInt64(8));

	/* write to stack */
	builder->StoreAt(pInt64,
	builder->	ConvertTo(pInt64, newSP), value);

	builder->StoreIndirect("VMFrame", "stack_ptr",
	builder->	Load("frame"),
	builder->	ConvertTo(pInt64, newSP));
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

int
SOMppMethod::getReceiverForSend(TR::IlBuilder *builder, VMSymbol* signature)
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

TR::IlValue *
SOMppMethod::getNumberOfIndexableFields(TR::IlBuilder *builder, TR::IlValue *array)
{
	TR::IlValue *objectSize = builder->LoadIndirect("VMObject", "objectSize", array);
	TR::IlValue *numberOfFields = builder->LoadIndirect("VMObject", "numberOfFields", array);

	TR::IlValue *extraSpace =
	builder->Sub(objectSize,
	builder->	Add(
	builder->		ConstInt64(sizeof(VMObject)),
	builder->		Mul(
	builder->			ConstInt64(sizeof(VMObject*)), numberOfFields)));

	TR::IlValue *numberOfIndexableFields =
	builder->Div(extraSpace,
	builder->	ConstInt64(sizeof(VMObject*)));

	return numberOfIndexableFields;
}

void
SOMppMethod::getIndexableFieldSlot(TR::IlBuilder *builder, TR::IlValue *array)
{
	TR::IlValue *numberOfFields = builder->LoadIndirect("VMObject", "numberOfFields", array);
	TR::IlValue *vmObjectSize = builder->ConstInt64(sizeof(VMObject));
	TR::IlValue *vmObjectPointerSize = builder->ConstInt64(sizeof(VMObject*));
	TR::IlValue *index =
	builder->Add(
	builder->	Load("indexValue"), numberOfFields);

	TR::IlValue *actualIndex =
	builder->Sub(index,
	builder->	ConstInt64(1));

	TR::IlValue *offset =
	builder->Add(vmObjectSize,
	builder->	Mul(actualIndex, vmObjectPointerSize));

	builder->Store("indexableFieldSlot",
	builder->	Add(array, offset));
}

/*************************************************
 * INLINE HELPERS
 *************************************************/

TR::IlBuilder *
SOMppMethod::verifyIntegerObject(TR::IlBuilder *builder, TR::IlValue *object, TR::IlBuilder **failPath)
{
	TR::IlBuilder *isIntegerPath = nullptr;

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

TR::IlBuilder *
SOMppMethod::getIntegerValue(TR::IlBuilder *builder, TR::IlValue *object, const char *valueName, TR::IlBuilder **failPath)
{
	TR::IlBuilder *isIntegerPath = verifyIntegerObject(builder, object, failPath);

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
SOMppMethod::createNewInteger(TR::IlBuilder *builder, TR::IlValue *integerValue)
{
#if USE_TAGGING
	TR::IlBuilder *lessThanMin = nullptr;
	TR::IlBuilder *possibleForTagging = nullptr;
	builder->IfThenElse(&lessThanMin, &possibleForTagging,
	builder->	GreaterThan(
	builder->		ConstInt64((int64_t)VMTAGGEDINTEGER_MIN), integerValue));

	lessThanMin->Store("newInteger",
	lessThanMin->	Call("newInteger", 1, integerValue));

	TR::IlBuilder *greaterThanMax = nullptr;
	TR::IlBuilder *tag = nullptr;
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
TR::IlBuilder *
SOMppMethod::generateILForIntergerOps(TR::BytecodeBuilder *builder, TR::IlBuilder **failPath)
{
	TR::IlValue *sp =
	builder->LoadIndirect("VMFrame", "stack_ptr",
	builder->	Load("frame"));

	TR::IlValue *rightObject =
	builder->LoadAt(pInt64,
	builder->	ConvertTo(pInt64, sp));

	TR::IlBuilder *successPath = getIntegerValue(builder, rightObject, "rightValueInteger", failPath);

	successPath->Store("spForIntegerOps",
	successPath->	Sub(sp,
	successPath->		ConstInt64(8)));

	TR::IlValue *leftObject =
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

TR::IlBuilder *
SOMppMethod::generateILForIntegerLessThan(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = nullptr;
	TR::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	TR::IlBuilder *thenPath = nullptr;
	TR::IlBuilder *elsePath = nullptr;
	isIntegerPath->IfThenElse(&thenPath, &elsePath,
	isIntegerPath->	LessThan(
	isIntegerPath->		Load("leftValueInteger"),
	isIntegerPath->		Load("rightValueInteger")));

	/* store true result */
	thenPath->StoreAt(pInt64,
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForIntegerOps")),
	thenPath->	ConstInt64((int64_t)trueObject));

	/* store false result */
	elsePath->StoreAt(pInt64,
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("spForIntegerOps")),
	elsePath->	ConstInt64((int64_t)falseObject));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForIntegerLessThanEqual(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = nullptr;
	TR::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	TR::IlBuilder *thenPath = nullptr;
	TR::IlBuilder *elsePath = nullptr;
	isIntegerPath->IfThenElse(&thenPath, &elsePath,
	isIntegerPath->	GreaterThan(
	isIntegerPath->		Load("leftValueInteger"),
	isIntegerPath->		Load("rightValueInteger")));

	/* store true result */
	thenPath->StoreAt(pInt64,
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForIntegerOps")),
	thenPath->	ConstInt64((int64_t)falseObject));

	/* store false result */
	elsePath->StoreAt(pInt64,
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("spForIntegerOps")),
	elsePath->	ConstInt64((int64_t)trueObject));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForIntegerGreaterThan(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = nullptr;
	TR::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	TR::IlBuilder *thenPath = nullptr;
	TR::IlBuilder *elsePath = nullptr;
	isIntegerPath->IfThenElse(&thenPath, &elsePath,
	isIntegerPath->	GreaterThan(
	isIntegerPath->		Load("leftValueInteger"),
	isIntegerPath->		Load("rightValueInteger")));

	/* store true result */
	thenPath->StoreAt(pInt64,
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForIntegerOps")),
	thenPath->	ConstInt64((int64_t)trueObject));

	/* store false result */
	elsePath->StoreAt(pInt64,
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("spForIntegerOps")),
	elsePath->	ConstInt64((int64_t)falseObject));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForIntegerGreaterThanEqual(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = nullptr;
	TR::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	TR::IlBuilder *thenPath = nullptr;
	TR::IlBuilder *elsePath = nullptr;
	isIntegerPath->IfThenElse(&thenPath, &elsePath,
	isIntegerPath->	LessThan(
	isIntegerPath->		Load("leftValueInteger"),
	isIntegerPath->		Load("rightValueInteger")));

	/* store true result */
	thenPath->StoreAt(pInt64,
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForIntegerOps")),
	thenPath->	ConstInt64((int64_t)falseObject));

	/* store false result */
	elsePath->StoreAt(pInt64,
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("spForIntegerOps")),
	elsePath->	ConstInt64((int64_t)trueObject));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForIntegerEqual(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = nullptr;
	TR::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	TR::IlBuilder *thenPath = nullptr;
	TR::IlBuilder *elsePath = nullptr;
	isIntegerPath->IfThenElse(&thenPath, &elsePath,
	isIntegerPath->	EqualTo(
	isIntegerPath->		Load("leftValueInteger"),
	isIntegerPath->		Load("rightValueInteger")));

	/* store true result */
	thenPath->StoreAt(pInt64,
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForIntegerOps")),
	thenPath->	ConstInt64((int64_t)trueObject));

	/* store false result */
	elsePath->StoreAt(pInt64,
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("spForIntegerOps")),
	elsePath->	ConstInt64((int64_t)falseObject));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForIntegerNotEqual(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = nullptr;
	TR::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	TR::IlBuilder *thenPath = nullptr;
	TR::IlBuilder *elsePath = nullptr;
	isIntegerPath->IfThenElse(&thenPath, &elsePath,
	isIntegerPath->	NotEqualTo(
	isIntegerPath->		Load("leftValueInteger"),
	isIntegerPath->		Load("rightValueInteger")));

	/* store true result */
	thenPath->StoreAt(pInt64,
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForIntegerOps")),
	thenPath->	ConstInt64((int64_t)trueObject));

	/* store false result */
	elsePath->StoreAt(pInt64,
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("spForIntegerOps")),
	elsePath->	ConstInt64((int64_t)falseObject));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForIntegerPlus(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = nullptr;
	TR::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	TR::IlValue *integerValue =
	isIntegerPath->Add(
	isIntegerPath->	Load("leftValueInteger"),
	isIntegerPath->	Load("rightValueInteger"));

	createNewInteger(isIntegerPath, integerValue);

	/* store new integer */
	isIntegerPath->StoreAt(pInt64,
	isIntegerPath->	ConvertTo(pInt64,
	isIntegerPath->		Load("spForIntegerOps")),
	isIntegerPath->	Load("newInteger"));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForIntegerMinus(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = nullptr;
	TR::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	TR::IlValue *integerValue =
	isIntegerPath->Sub(
	isIntegerPath->	Load("leftValueInteger"),
	isIntegerPath->	Load("rightValueInteger"));

	createNewInteger(isIntegerPath, integerValue);

	/* store new integer */
	isIntegerPath->StoreAt(pInt64,
	isIntegerPath->	ConvertTo(pInt64,
	isIntegerPath->		Load("spForIntegerOps")),
	isIntegerPath->	Load("newInteger"));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForIntegerStar(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = nullptr;
	TR::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	TR::IlValue *integerValue =
	isIntegerPath->Mul(
	isIntegerPath->	Load("leftValueInteger"),
	isIntegerPath->	Load("rightValueInteger"));

	createNewInteger(isIntegerPath, integerValue);

	/* store new integer */
	isIntegerPath->StoreAt(pInt64,
	isIntegerPath->	ConvertTo(pInt64,
	isIntegerPath->		Load("spForIntegerOps")),
	isIntegerPath->	Load("newInteger"));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForIntegerValue(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = nullptr;
	TR::IlValue *sp =
	builder->LoadIndirect("VMFrame", "stack_ptr",
	builder->	Load("frame"));

	TR::IlValue *currentObject =
	builder->LoadAt(pInt64,
	builder->	ConvertTo(pInt64, sp));

	/* If it is an integer there is nothing to do */
	verifyIntegerObject(builder, currentObject, &failInline);

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForIntegerMax(TR::BytecodeBuilder *builder)
{
	/* TODO fix because rightObject does not exist */
	TR::IlBuilder *failInline = nullptr;
	TR::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	TR::IlBuilder *thenPath = nullptr;
	isIntegerPath->IfThen(&thenPath,
	isIntegerPath->	LessThan(
	isIntegerPath->		Load("leftValueInteger"),
	isIntegerPath->		Load("rightValueInteger")));

	/* store rightObject as it is the max. If leftObject (receiver) is the max value
	 * there is nothing to do since it is already the value at the right location on
	 * the stack */
	thenPath->StoreAt(pInt64,
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForIntegerOps")),
	thenPath->	Load("rightObject"));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForIntegerNegated(TR::BytecodeBuilder *builder)
{
	TR::IlValue *sp =
	builder->LoadIndirect("VMFrame", "stack_ptr",
	builder->	Load("frame"));

	TR::IlValue *integerObject =
	builder->LoadAt(pInt64,
	builder->	ConvertTo(pInt64, sp));

	TR::IlBuilder *failInline = nullptr;
	TR::IlBuilder *isIntegerPath = getIntegerValue(builder, integerObject, "integerValue", &failInline);

	TR::IlValue *integerValue =
	isIntegerPath->Sub(
	isIntegerPath->	ConstInt64(0),
	isIntegerPath->	Load("integerValue"));

	createNewInteger(isIntegerPath, integerValue);

	/* store new integer */
	isIntegerPath->StoreAt(pInt64,
	isIntegerPath->	ConvertTo(pInt64, sp),
	isIntegerPath->	Load("newInteger"));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForIntegerAbs(TR::BytecodeBuilder *builder)
{
	TR::IlValue *sp =
	builder->LoadIndirect("VMFrame", "stack_ptr",
	builder->	Load("frame"));

	TR::IlValue *integerObject =
	builder->LoadAt(pInt64,
	builder->	ConvertTo(pInt64, sp));

	TR::IlBuilder *failInline = nullptr;
	TR::IlBuilder *isIntegerPath = getIntegerValue(builder, integerObject, "integerValue", &failInline);

	TR::IlBuilder *isNegative = nullptr;
	isIntegerPath->IfThen(&isNegative,
	isIntegerPath->	LessThan(
	isIntegerPath->		Load("integerValue"),
	isIntegerPath->		ConstInt64(0)));

	TR::IlValue *integerValue =
	isNegative->Sub(
	isNegative->	ConstInt64(0),
	isNegative->	Load("integerValue"));

	createNewInteger(isNegative, integerValue);

	/* store new integer */
	isNegative->StoreAt(pInt64,
	isNegative->	ConvertTo(pInt64, sp),
	isNegative->	Load("newInteger"));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForArrayAt(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = nullptr;

	TR::IlValue *sp =
	builder->LoadIndirect("VMFrame", "stack_ptr",
	builder->	Load("frame"));

	TR::IlValue *indexObject =
	builder->LoadAt(pInt64,
	builder->	ConvertTo(pInt64, sp));

	TR::IlBuilder *indexIsInteger = getIntegerValue(builder, indexObject, "indexValue", &failInline);

	TR::IlValue * spForReceiver =
	indexIsInteger->Sub(sp,
	indexIsInteger->	ConstInt64(8));

	TR::IlValue *arrayObject =
	indexIsInteger->LoadAt(pInt64,
	indexIsInteger->	ConvertTo(pInt64, spForReceiver));

	indexIsInteger->Store("arrayClass",
	indexIsInteger->	LoadIndirect("VMObject", "clazz", arrayObject));

	TR::IlBuilder *isArrayPath = nullptr;
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
	isArrayPath->StoreAt(pInt64,
	isArrayPath->	ConvertTo(pInt64, spForReceiver),
	isArrayPath->	Load("result"));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForArrayAtPut(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = nullptr;

	TR::IlValue *sp =
	builder->LoadIndirect("VMFrame", "stack_ptr",
	builder->	Load("frame"));

	builder->Store("valueObject",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64, sp)));

	TR::IlValue *indexObject =
	builder->LoadAt(pInt64,
	builder->	ConvertTo(pInt64,
	builder->		Sub(sp,
	builder->			ConstInt64(8))));

	TR::IlBuilder *indexIsInteger = getIntegerValue(builder, indexObject, "indexValue", &failInline);

	TR::IlValue * spForReceiver =
	indexIsInteger->Sub(sp,
	indexIsInteger->	ConstInt64(16));

	/* peek the array object without popping */
	TR::IlValue *arrayObject =
	indexIsInteger->LoadAt(pInt64,
	indexIsInteger->	ConvertTo(pInt64, spForReceiver));

	indexIsInteger->Store("arrayClass",
	indexIsInteger->	LoadIndirect("VMObject", "clazz", arrayObject));

	TR::IlBuilder *isArrayPath = nullptr;
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
	isArrayPath->StoreAt(pInt64,
	isArrayPath->	ConvertTo(pInt64,
	isArrayPath->		Load("indexableFieldSlot")),
	isArrayPath->	Load("valueObject"));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForArrayLength(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = nullptr;

	TR::IlValue *sp =
	builder->LoadIndirect("VMFrame", "stack_ptr",
	builder->	Load("frame"));

	TR::IlValue *arrayObject =
	builder->LoadAt(pInt64,
	builder->	ConvertTo(pInt64, sp));

	builder->Store("arrayClass",
	builder->	Call("getClass", 1, arrayObject));

	TR::IlBuilder *isArrayPath = nullptr;

	builder->IfThenElse(&isArrayPath, &failInline,
	builder->	EqualTo(
	builder->		Load("arrayClass"),
	builder->		ConstInt64((int64_t)arrayClass)));

	createNewInteger(isArrayPath, getNumberOfIndexableFields(isArrayPath, arrayObject));

	/* store new integer */
	isArrayPath->StoreAt(pInt64,
	isArrayPath->	ConvertTo(pInt64, sp),
	isArrayPath->	Load("newInteger"));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForNilisNil(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *isNil = NULL;
	TR::IlBuilder *notNil = NULL;

	TR::IlValue *sp =
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

	isNil->StoreAt(pInt64,
	isNil->	ConvertTo(pInt64, sp),
	isNil-> LoadAt(pInt64,
	isNil->		ConvertTo(pInt64,
	isNil->			ConstInt64((int64_t)&trueObject))));

	notNil->StoreAt(pInt64,
	notNil->	ConvertTo(pInt64, sp),
	notNil-> LoadAt(pInt64,
	notNil->	ConvertTo(pInt64,
	notNil->		ConstInt64((int64_t)&falseObject))));

	return nullptr;
}

TR::IlBuilder *
SOMppMethod::generateILForBooleanNot(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *isTrue = NULL;
	TR::IlBuilder *notTrue = NULL;

	TR::IlValue *sp =
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

	isTrue->StoreAt(pInt64,
	isTrue->	ConvertTo(pInt64, sp),
	isTrue->	Load("falseObject"));

	TR::IlBuilder *isFalse = NULL;
	TR::IlBuilder *failInline = NULL;

	notTrue->IfThenElse(&isFalse, &failInline,
	notTrue->	EqualTo(
	notTrue->		Load("currentObject"),
	notTrue->		Load("falseObject")));

	isFalse->StoreAt(pInt64,
	isFalse->	ConvertTo(pInt64, sp),
	isFalse->	Load("trueObject"));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForDoubleOps(TR::BytecodeBuilder *builder, TR::IlBuilder **failPath)
{
	TR::IlValue *sp =
	builder->LoadIndirect("VMFrame", "stack_ptr",
	builder->	Load("frame"));

	builder->Store("rightObjectDouble",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64, sp)));

	builder->Store("rightClassDouble",
	builder->	Call("getClass", 1,
	builder->		Load("rightObjectDouble")));

	TR::IlBuilder *rightIsDouble = NULL;
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

	TR::IlBuilder *isDoublePath = NULL;
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

TR::IlBuilder *
SOMppMethod::generateILForDoubleLessThan(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = NULL;
	TR::IlBuilder *isDoublePath = generateILForDoubleOps(builder, &failInline);

	TR::IlBuilder *thenPath = NULL;
	TR::IlBuilder *elsePath = NULL;
	isDoublePath->IfThenElse(&thenPath, &elsePath,
	isDoublePath->	LessThan(
	isDoublePath->		Load("leftValueDouble"),
	isDoublePath->		Load("rightValueDouble")));

	/* store true result */
	thenPath->StoreAt(pInt64,
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForDoubleOps")),
	thenPath->	ConstInt64((int64_t)trueObject));

	/* store false result */
	elsePath->StoreAt(pInt64,
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("spForDoubleOps")),
	elsePath->	ConstInt64((int64_t)falseObject));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForDoubleLessThanEqual(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = NULL;
	TR::IlBuilder *isDoublePath = generateILForDoubleOps(builder, &failInline);

	TR::IlBuilder *thenPath = NULL;
	TR::IlBuilder *elsePath = NULL;
	isDoublePath->IfThenElse(&thenPath, &elsePath,
	isDoublePath->	GreaterThan(
	isDoublePath->		Load("leftValueDouble"),
	isDoublePath->		Load("rightValueDouble")));

	/* store true result */
	thenPath->StoreAt(pInt64,
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForDoubleOps")),
	thenPath->	ConstInt64((int64_t)falseObject));

	/* store false result */
	elsePath->StoreAt(pInt64,
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("spForDoubleOps")),
	elsePath->	ConstInt64((int64_t)trueObject));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForDoubleGreaterThan(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = NULL;
	TR::IlBuilder *isDoublePath = generateILForDoubleOps(builder, &failInline);

	TR::IlBuilder *thenPath = NULL;
	TR::IlBuilder *elsePath = NULL;
	isDoublePath->IfThenElse(&thenPath, &elsePath,
	isDoublePath->	GreaterThan(
	isDoublePath->		Load("leftValueDouble"),
	isDoublePath->		Load("rightValueDouble")));

	/* store true result */
	thenPath->StoreAt(pInt64,
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForDoubleOps")),
	thenPath->	ConstInt64((int64_t)trueObject));

	/* store false result */
	elsePath->StoreAt(pInt64,
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("spForDoubleOps")),
	elsePath->	ConstInt64((int64_t)falseObject));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForDoubleGreaterThanEqual(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = NULL;
	TR::IlBuilder *isDoublePath = generateILForDoubleOps(builder, &failInline);

	TR::IlBuilder *thenPath = NULL;
	TR::IlBuilder *elsePath = NULL;
	isDoublePath->IfThenElse(&thenPath, &elsePath,
	isDoublePath->	LessThan(
	isDoublePath->		Load("leftValueDouble"),
	isDoublePath->		Load("rightValueDouble")));

	/* store true result */
	thenPath->StoreAt(pInt64,
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForDoubleOps")),
	thenPath->	ConstInt64((int64_t)falseObject));

	/* store false result */
	elsePath->StoreAt(pInt64,
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("spForDoubleOps")),
	elsePath->	ConstInt64((int64_t)trueObject));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForDoubleEqual(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = NULL;
	TR::IlBuilder *isDoublePath = generateILForDoubleOps(builder, &failInline);

	TR::IlBuilder *thenPath = NULL;
	TR::IlBuilder *elsePath = NULL;
	isDoublePath->IfThenElse(&thenPath, &elsePath,
	isDoublePath->	EqualTo(
	isDoublePath->		Load("leftValueDouble"),
	isDoublePath->		Load("rightValueDouble")));

	/* store true result */
	thenPath->StoreAt(pInt64,
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForDoubleOps")),
	thenPath->	ConstInt64((int64_t)trueObject));

	/* store false result */
	elsePath->StoreAt(pInt64,
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("spForDoubleOps")),
	elsePath->	ConstInt64((int64_t)falseObject));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForDoubleNotEqual(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = NULL;
	TR::IlBuilder *isDoublePath = generateILForDoubleOps(builder, &failInline);

	TR::IlBuilder *thenPath = NULL;
	TR::IlBuilder *elsePath = NULL;
	isDoublePath->IfThenElse(&thenPath, &elsePath,
	isDoublePath->	EqualTo(
	isDoublePath->		Load("leftValueDouble"),
	isDoublePath->		Load("rightValueDouble")));

	/* store true result */
	thenPath->StoreAt(pInt64,
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("spForDoubleOps")),
	thenPath->	ConstInt64((int64_t)falseObject));

	/* store false result */
	elsePath->StoreAt(pInt64,
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("spForDoubleOps")),
	elsePath->	ConstInt64((int64_t)trueObject));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForDoublePlus(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = NULL;
	TR::IlBuilder *isDoublePath = generateILForDoubleOps(builder, &failInline);

	isDoublePath->StoreAt(pInt64,
	isDoublePath->	ConvertTo(pInt64,
	isDoublePath->		Load("spForDoubleOps")),
	isDoublePath->	Call("newDouble", 1,
	isDoublePath->		Add(
	isDoublePath->			Load("leftValueDouble"),
	isDoublePath->			Load("rightValueDouble"))));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForDoubleMinus(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = NULL;
	TR::IlBuilder *isDoublePath = generateILForDoubleOps(builder, &failInline);

	isDoublePath->StoreAt(pInt64,
	isDoublePath->	ConvertTo(pInt64,
	isDoublePath->		Load("spForDoubleOps")),
	isDoublePath->	Call("newDouble", 1,
	isDoublePath->		Sub(
	isDoublePath->			Load("leftValueDouble"),
	isDoublePath->			Load("rightValueDouble"))));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForDoubleStar(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = NULL;
	TR::IlBuilder *isDoublePath = generateILForDoubleOps(builder, &failInline);

	isDoublePath->StoreAt(pInt64,
	isDoublePath->	ConvertTo(pInt64,
	isDoublePath->		Load("spForDoubleOps")),
	isDoublePath->	Call("newDouble", 1,
	isDoublePath->		Mul(
	isDoublePath->			Load("leftValueDouble"),
	isDoublePath->			Load("rightValueDouble"))));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForDoubleSlashSlash(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = NULL;
	TR::IlBuilder *isDoublePath = generateILForDoubleOps(builder, &failInline);

	isDoublePath->StoreAt(pInt64,
	isDoublePath->	ConvertTo(pInt64,
	isDoublePath->		Load("spForDoubleOps")),
	isDoublePath->	Call("newDouble", 1,
	isDoublePath->		Div(
	isDoublePath->			Load("leftValueDouble"),
	isDoublePath->			Load("rightValueDouble"))));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::doInlineIfPossible(TR::BytecodeBuilder *builder, VMSymbol* signature, long bytecodeIndex)
{
	VMClass *receiverFromCache = method->getInvokeReceiverCache(bytecodeIndex);
	char *signatureChars = signature->GetChars();
	TR::IlBuilder *genericSend = builder;

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

TR::IlBuilder *
SOMppMethod::generateRecognizedMethod(TR::BytecodeBuilder *builder, VMClass *receiverFromCache, char *signatureChars)
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

TR::IlBuilder *
SOMppMethod::generateGenericInline(TR::BytecodeBuilder *builder, VMClass *receiverFromCache, VMMethod *vmMethod, char *signatureChars)
{
	TR::BytecodeBuilder *initialBilder = builder;
	TR::IlBuilder *fail = nullptr;

	/* check to see if there is room on the current stack to execute the method */
	/* spaceAvailable = # of stack slots in the current method - current depth + # of arguments for the method to inline */
	long methodToInlineNumberOfArguments = vmMethod->GetNumberOfArguments();
	long spaceAvailable = method->GetMaximumNumberOfStackElements() - currentStackDepth + methodToInlineNumberOfArguments;
	if (vmMethod->GetMaximumNumberOfStackElements() <= spaceAvailable) {
		if (methodIsInlineable(vmMethod)) {
			TR::IlBuilder *inlineBuilder = nullptr;
			/* TODO if debug mode gen code to ensure stack has room at runtime */

			/* make sure the receiver class is the right one */
			initialBilder->IfThenElse(&inlineBuilder, &fail,
			initialBilder->	EqualTo(
			initialBilder->		ConstInt64((int64_t)receiverFromCache),
			initialBilder->		Load("receiverClass")));

			inlineBuilder->Store("argumentsArray",
			inlineBuilder->	CreateLocalArray((int32_t)methodToInlineNumberOfArguments, Int64));

			for (int32_t i = 0; i < (int32_t)methodToInlineNumberOfArguments; i++) {
				inlineBuilder->StoreAt(pInt64,
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

					TR::IlValue *fieldValue =
					inlineBuilder->Call("getFieldFrom", 2,
					inlineBuilder->	Load("receiverObject"),
					inlineBuilder->	ConstInt64((int64_t)fieldIndex));

					push(inlineBuilder, fieldValue);

					break;
				}
				case BC_PUSH_CONSTANT:
				{
					uint8_t valueOffset = vmMethod->GetBytecode(bytecodeIndex + 1);

					TR::IlValue *constant =
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

					TR::IlValue *global =
					inlineBuilder->Call("getGlobal", 1,
					inlineBuilder->	ConstInt64((int64_t)globalName));

					push(inlineBuilder, global);

					break;
				}
				case BC_PUSH_ARGUMENT:
				{
					uint8_t argumentIndex = vmMethod->GetBytecode(bytecodeIndex + 1);

					TR::IlValue *argument =
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

					TR::IlValue *value = peek(inlineBuilder);
					pop(inlineBuilder);

					inlineBuilder->Call("setFieldTo", 3,
					inlineBuilder->	Load("receiverObject"),
					inlineBuilder->	ConstInt64((int64_t)fieldIndex), value);

					break;
				}
				case BC_RETURN_LOCAL:
				{
					TR::IlValue *returnValue = peek(inlineBuilder);

					/* store the return value */
					inlineBuilder->StoreAt(pInt64,
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
