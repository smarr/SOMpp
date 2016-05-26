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
printInt32(int32_t value)
{
#define PRINTINT32_LINE LINETOSTR(__LINE__)
	fprintf(stderr, "%d", value);
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

	DefineName(methodName);
	DefineReturnType(Int64);

	defineStructures(types);
	defineParameters();
	defineLocals();
	defineFunctions();
}

void
SOMppMethod::defineParameters()
{
	DefineParameter("interpreter", Int64);
	DefineParameter("frame", pInt64);
	DefineParameter("stackPtrPtr", pInt64);
}

void
SOMppMethod::defineLocals()
{
}

void
SOMppMethod::defineStructures(TR::TypeDictionary *types)
{
	pInt64 = types->PointerTo(Int64);
	pDouble = types->PointerTo(Double);

	defineVMFrameStructure(types);
}

void
SOMppMethod::defineFunctions()
{
	DefineFunction((char *)"printString", (char *)__FILE__, (char *)PRINTSTRING_LINE, (void *)&printString, NoType, 1, Int64);
	DefineFunction((char *)"printInt32", (char *)__FILE__, (char *)PRINTINT32_LINE, (void *)&printInt32, NoType, 1, Int32);
	DefineFunction((char *)"printInt64", (char *)__FILE__, (char *)PRINTINT64_LINE, (void *)&printInt64, NoType, 1, Int64);
	DefineFunction((char *)"printInt64Hex", (char *)__FILE__, (char *)PRINTINT64HEX_LINE, (void *) &printInt64Hex, NoType, 1, Int64);
	DefineFunction((char *)"getClass", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::GET_CLASS_LINE, (void *)&BytecodeHelper::getClass, Int64, 1, Int64);
	DefineFunction((char *)"getGlobal", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::GET_GLOBAL_LINE, (void *)&BytecodeHelper::getGlobal, Int64, 1, Int64);
	DefineFunction((char *)"getNewBlock", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::GET_NEW_BLOCK_LINE, (void *)&BytecodeHelper::getNewBlock, Int64, 3, Int64, Int64, Int64);
	DefineFunction((char *)"getLocal", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::GET_LOCAL_LINE, (void *)&BytecodeHelper::getLocal, Int64, 3, Int64, Int64, Int64);
	DefineFunction((char *)"setLocal", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::SET_LOCAL_LINE, (void *)&BytecodeHelper::setLocal, NoType, 4, Int64, Int64, Int64, Int64);
	DefineFunction((char *)"getArgument", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::GET_ARGUMENT_LINE, (void *)&BytecodeHelper::getArgument, Int64, 3, Int64, Int64, Int64);
	DefineFunction((char *)"newInteger", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::NEW_INTEGER_LINE, (void *)&BytecodeHelper::newInteger, Int64, 1, Int64);
	DefineFunction((char *)"getIndexableField", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::GET_INDEXABLE_FIELD_LINE, (void *)&BytecodeHelper::getIndexableField, Int64, 2, Int64, Int64);
	DefineFunction((char *)"setIndexableField", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::SET_INDEXABLE_FIELD_LINE, (void *)&BytecodeHelper::setIndexableField, NoType, 3, Int64, Int64, Int64);
	DefineFunction((char *)"getNumberOfIndexableFields", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::GET_NUMBER_OF_INDEXABLE_FIELDS_LINE, (void *)&BytecodeHelper::getNumberOfIndexableFields, Int64, 1, Int64);
	DefineFunction((char *)"getFieldFrom", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::GET_FIELD_FROM_LINE, (void *)&BytecodeHelper::getFieldFrom, Int64, 2, Int64, Int64);
	DefineFunction((char *)"getField", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::GET_FIELD_LINE, (void *)&BytecodeHelper::getField, Int64, 2, Int64, Int64);
	DefineFunction((char *)"setFieldTo", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::SET_FIELD_TO_LINE, (void *)&BytecodeHelper::setFieldTo, NoType, 3, Int64, Int64, Int64);
	DefineFunction((char *)"setField", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::SET_FIELD_LINE, (void *)&BytecodeHelper::setField, NoType, 3, Int64, Int64, Int64);
	DefineFunction((char *)"getInvokable", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::GET_INVOKABLE_LINE, (void *)&BytecodeHelper::getInvokable, Int64, 2, Int64, Int64);
	DefineFunction((char *)"doSendIfRequired", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::DO_SEND_IF_REQUIRED_LINE, (void *)&BytecodeHelper::doSendIfRequired, Int64, 7, Int64, Int64, Int64, Int64, Int64, Int64, Int64);
	DefineFunction((char *)"doSuperSendHelper", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::DO_SUPER_SEND_HELPER_LINE, (void *)&BytecodeHelper::doSuperSendHelper, Int64, 4, Int64, Int64, Int64, Int64);
	DefineFunction((char *)"popFrameAndPushResult", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::POP_FRAME_AND_PUSH_RESULT_LINE, (void *)&BytecodeHelper::popFrameAndPushResult, NoType, 3, Int64, Int64, Int64);
	DefineFunction((char *)"popToContext", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::POP_TO_CONTEXT_LINE, (void *)&BytecodeHelper::popToContext, Int64, 2, Int64, Int64);
	DefineFunction((char *)"verifyArgument", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::VERIFY_ARGUMENT_LINE, (void *)&BytecodeHelper::verifyArgument, NoType, 4, Int64, Int64, Int64, Int64);
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
	TR::BytecodeBuilder **bytecodeBuilderTable = NULL;
	bool canHandle = true;
	int64_t i = 0;

	Store("spPtr",
		Load("stackPtrPtr"));

	/* Fetch the actual stack pointer*/
	Store("sp",
		LoadAt(pInt64,
				Load("spPtr")));

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
	/* read from sp */
	builder->Store("value",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			Load("sp"))));

	/* increment stack */
	builder->Store("sp",
	builder->	Add(
	builder->		Load("sp"),
	builder->		ConstInt64(8)));

	/* write to stack */
	builder->StoreAt(pInt64,
	builder->	ConvertTo(pInt64,
	builder->		Load("sp")),
	builder->	Load("value"));

	currentStackDepth += 1;
}

void
SOMppMethod::doPushLocal(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	uint8_t index = method->GetBytecode(bytecodeIndex + 1);
	uint8_t level = method->GetBytecode(bytecodeIndex + 2);

	const char *contextName = NULL;

	if (level == 0) {
		contextName = "frame";
	} else {
		contextName = "context";

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
	}

	/* increment stack */
	builder->Store("sp",
	builder->	Add(
	builder->		Load("sp"),
	builder->		ConstInt64(8)));

	/* write local to stack */
	builder->StoreAt(pInt64,
	builder->	ConvertTo(pInt64,
	builder->		Load("sp")),
	builder->	LoadAt(pInt64,
	builder->		IndexAt(pInt64,
	builder->			LoadIndirect("VMFrame", "locals",
	builder->				Load(contextName)),
	builder->			ConstInt64(index))));

	currentStackDepth += 1;
}

void
SOMppMethod::doPushArgument(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	uint8_t index = method->GetBytecode(bytecodeIndex + 1);
	uint8_t level = method->GetBytecode(bytecodeIndex + 2);

	const char *contextName = NULL;

	if (level == 0) {
		contextName = "frame";
	} else {
		contextName = "context";

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
	}

	/* increment stack */
	builder->Store("sp",
	builder->	Add(
	builder->		Load("sp"),
	builder->		ConstInt64(8)));

	/* write argument to stack */
	builder->StoreAt(pInt64,
	builder->	ConvertTo(pInt64,
	builder->		Load("sp")),
	builder->	LoadAt(pInt64,
	builder->		IndexAt(pInt64,
	builder->			LoadIndirect("VMFrame", "arguments",
	builder->				Load(contextName)),
	builder->			ConstInt64(index))));

	currentStackDepth += 1;
}

void
SOMppMethod::doPushField(TR::BytecodeBuilder *builder, VMMethod *currentMethod, long bytecodeIndex)
{
	uint8_t fieldIndex = currentMethod->GetBytecode(bytecodeIndex + 1);

	builder->Store("field",
	builder->	Call("getField", 2,
	builder->		Load("frame"),
	builder->		ConstInt64((int64_t)fieldIndex)));

	/* increment stack */
	builder->Store("sp",
	builder->	Add(
	builder->		Load("sp"),
	builder->		ConstInt64(8)));

	/* write field to stack */
	builder->StoreAt(pInt64,
	builder->	ConvertTo(pInt64,
	builder->		Load("sp")),
	builder->	Load("field"));

	currentStackDepth += 1;
}

void
SOMppMethod::doPushBlock(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	/* TODO come back and handle optimization in Interpreter::doPushBlock */
	VMMethod* blockMethod = static_cast<VMMethod*>(method->GetConstant(bytecodeIndex));
	long numOfArgs = blockMethod->GetNumberOfArguments();

	builder->Store("block",
	builder->	Call("getNewBlock", 3,
	builder->		Load("frame"),
	builder->		ConstInt64((int64_t)blockMethod),
	builder->		ConstInt64((int64_t)numOfArgs)));

	/* increment stack */
	builder->Store("sp",
	builder->	Add(
	builder->		Load("sp"),
	builder->		ConstInt64(8)));

	/* write block to stack */
	builder->StoreAt(pInt64,
	builder->	ConvertTo(pInt64,
	builder->		Load("sp")),
	builder->	Load("block"));

	currentStackDepth += 1;
}

void
SOMppMethod::doPushConstant(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	uint8_t valueOffset = method->GetBytecode(bytecodeIndex + 1);

	/* increment stack */
	builder->Store("sp",
	builder->	Add(
	builder->		Load("sp"),
	builder->		ConstInt64(8)));

	/* write constant to stack */
	builder->StoreAt(pInt64,
	builder->	ConvertTo(pInt64,
	builder->		Load("sp")),
	builder->	LoadAt(pInt64,
	builder->		IndexAt(pInt64,
	builder->			ConvertTo(pInt64,
	builder->				ConstInt64((int64_t)method->indexableFields)),
	builder->			ConstInt64(valueOffset))));

	currentStackDepth += 1;
}

void
SOMppMethod::doPushGlobal(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	/* TODO If objects can move this is a runtime and not compile time fetch */
	VMSymbol* globalName = static_cast<VMSymbol*>(method->GetConstant(bytecodeIndex));

	builder->Store("globalValue",
	builder->	Call("getGlobal", 1,
	builder->		ConstInt64((int64_t)globalName)));

	TR::IlBuilder *globalIsNullPtr = NULL;
	builder->IfThen(&globalIsNullPtr,
	builder->	EqualTo(
	builder->		Load("globalValue"),
	builder->		ConstInt64((int64_t)nullptr)));

	/* TODO Come back and handle */
	globalIsNullPtr->Call("printString", 1,
	globalIsNullPtr->	ConstInt64((int64_t)"\n\n\n doPushGlobal crashing due to unknown global\n\n\n\n"));
	globalIsNullPtr->StoreAt(pInt64,
	globalIsNullPtr->	ConvertTo(pInt64,
	globalIsNullPtr->		ConstInt64(0)),
	globalIsNullPtr->	ConstInt64(0));

	justReturn(globalIsNullPtr);

	/* increment stack */
	builder->Store("sp",
	builder->	Add(
	builder->		Load("sp"),
	builder->		ConstInt64(8)));

	/* write global to stack */
	builder->StoreAt(pInt64,
	builder->	ConvertTo(pInt64,
	builder->		Load("sp")),
	builder->	Load("globalValue"));

	currentStackDepth += 1;
}

void
SOMppMethod::doPop(TR::BytecodeBuilder *builder)
{
	/* decrement stack */
	builder->Store("sp",
	builder->	Add(
	builder->		Load("sp"),
	builder->		ConstInt64(-8)));

	currentStackDepth -= 1;
}

void
SOMppMethod::doPopLocal(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	uint8_t index = method->GetBytecode(bytecodeIndex + 1);
	uint8_t level = method->GetBytecode(bytecodeIndex + 2);

	/* read value from stack */
	builder->Store("value1",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			Load("sp"))));

	/* decrement stack */
	builder->Store("sp",
	builder->	Sub(
	builder->		Load("sp"),
	builder->		ConstInt64(8)));

	const char* contextName = NULL;

	if (level == 0) {
		contextName = "frame";
	} else {
		contextName = "context";

		builder->Store("context",
		builder->	Load("frame"));

		if (level == 1) {
			builder->Store("context",
			builder->	LoadIndirect("VMFrame", "context",
			builder->		Load("context")));
		} else if (level > 1) {
			TR::IlBuilder *iloop = NULL;
			builder->ForLoopUp("i", &iloop,
			builder->	ConstInt32(0),
			builder->	ConstInt32(level),
			builder->	ConstInt32(1));

			iloop->Store("context",
			iloop->	LoadIndirect("VMFrame", "context",
			iloop->		Load("context")));
		}
	}

	builder->StoreAt(pInt64,
	builder->	IndexAt(pInt64,
	builder->		LoadIndirect("VMFrame", "locals",
	builder->			Load(contextName)),
	builder->		ConstInt64(index)),
	builder->	Load("value1"));

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

	/* read value from stack */
	builder->Store("value1",
	builder->	LoadAt(pInt64,
	builder->		Load("sp")));

	/* decrement stack */
	builder->Store("sp",
	builder->	Sub(
	builder->		Load("sp"),
	builder->		ConstInt64(8)));

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
	builder->		ConstInt64(index)),
	builder->	Load("value1"));

	currentStackDepth -= 1;
}

void
SOMppMethod::doPopField(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	uint8_t fieldIndex = method->GetBytecode(bytecodeIndex + 1);

	builder->StoreAt(pInt64,
	builder->	Load("spPtr"),
	builder->	Load("sp"));

	builder->Call("setField", 3,
	builder->	Load("frame"),
	builder->	ConstInt64((int64_t)fieldIndex),
	builder->	Load("sp"));

	builder->Store("sp",
	builder->	LoadAt(pInt64,
	builder->		Load("spPtr")));

	currentStackDepth -= 1;
}



void
SOMppMethod::doSend(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex)
{
	VMSymbol* signature = static_cast<VMSymbol*>(method->GetConstant(bytecodeIndex));
	int numOfArgs = Signature::GetNumberOfArguments(signature);

	builder->Store("receiverAddress",
	builder->	Add(
	builder->		Load("sp"),
	builder->		ConstInt64(((int64_t)numOfArgs - 1) * -8)));

	builder->Store("receiverObject",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			Load("receiverAddress"))));

	builder->Store("receiverClass",
	builder->	Call("getClass", 1,
	builder->		Load("receiverObject")));

	/* Check for inline now that I have receiver and receiverClass */
	/* They are needed for both the generic send and inline path */
	TR::IlBuilder *sendBuilder = doInlineIfPossible(builder, signature, bytecodeIndex);

	/* NULL means that there is no failure case so no generic handling*/
	if (NULL != sendBuilder) {
		sendBuilder->Store("invokable",
		sendBuilder->	Call("getInvokable", 2,
		sendBuilder->		Load("receiverClass"),
		sendBuilder->		ConstInt64((int64_t)signature)));

		sendBuilder->StoreAt(pInt64,
		sendBuilder->	Load("spPtr"),
		sendBuilder->	Load("sp"));

		sendBuilder->Store("return",
		sendBuilder->	Call("doSendIfRequired", 7,
		sendBuilder->		Load("interpreter"),
		sendBuilder->		Load("frame"),
		sendBuilder->		Load("invokable"),
		sendBuilder->		ConstInt64((int64_t)bytecodeIndex),
		sendBuilder->		Load("receiverObject"),
		sendBuilder->		Load("receiverAddress"),
		sendBuilder->		ConstInt64((int64_t)numOfArgs)));

		TR::IlBuilder *bail = NULL;
		sendBuilder->IfThen(&bail,
		sendBuilder->	EqualTo(
		sendBuilder->		Load("return"),
		sendBuilder->		ConstInt64(-1)));

		justReturn(bail);

		sendBuilder->Store("sp",
		sendBuilder->	LoadAt(pInt64,
		sendBuilder->		Load("spPtr")));

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

	builder->StoreAt(pInt64,
	builder->	Load("spPtr"),
	builder->	Load("sp"));

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

	builder->Store("sp",
	builder->	LoadAt(pInt64,
	builder->		Load("spPtr")));

	TR::IlBuilder *start = (TR::IlBuilder *)bytecodeBuilderTable[0];
	builder->IfCmpNotEqual(&start,
	builder->	Load("return"),
	builder->	ConstInt64((int64_t)bytecodeIndex));

	currentStackDepth -= (numOfArgs - 1);
}

void
SOMppMethod::doReturnLocal(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	builder->Store("result",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			Load("sp"))));

	builder->StoreAt(pInt64,
	builder->	Load("spPtr"),
	builder->		Add(
	builder->			Load("sp"),
	builder->			ConstInt64(-8)));

	builder->Call("popFrameAndPushResult", 3,
	builder->	Load("interpreter"),
	builder->	Load("frame"),
	builder->	Load("result"));

	justReturn(builder);
	/* do not adjust currentStackDepth */
}

void
SOMppMethod::doReturnNonLocal(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	builder->Store("result",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			Load("sp"))));

	builder->StoreAt(pInt64,
	builder->	Load("spPtr"),
	builder->		Add(
	builder->			Load("sp"),
	builder->			ConstInt64(-8)));

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
	continuePath->	Load("frame"),
	continuePath->	Load("result"));

	justReturn(builder);
	/* do not adjust currentStackDepth */
}

void
SOMppMethod::doJumpIfFalse(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex)
{
	/* read value from stack */
	builder->Store("value1",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			Load("sp"))));

	/* decrement stack */
	builder->Store("sp",
	builder->	Sub(
	builder->		Load("sp"),
	builder->		ConstInt64(8)));

	TR::BytecodeBuilder *destBuilder = bytecodeBuilderTable[calculateBytecodeIndexForJump(bytecodeIndex)];
	builder->AddSuccessorBuilder(destBuilder);

	TR::IlBuilder *dest = (TR::IlBuilder *)destBuilder;
	builder->IfCmpEqual(&dest,
	builder->	Load("value1"),
	builder->	ConstInt64((int64_t)falseObject));

	currentStackDepth -= 1;
}

void
SOMppMethod::doJumpIfTrue(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex)
{
	/* read value from stack */
	builder->Store("value1",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			Load("sp"))));

	/* decrement stack */
	builder->Store("sp",
	builder->	Sub(
	builder->		Load("sp"),
	builder->		ConstInt64(8)));

	TR::BytecodeBuilder *destBuilder = bytecodeBuilderTable[calculateBytecodeIndexForJump(bytecodeIndex)];
	builder->AddSuccessorBuilder(destBuilder);

	TR::IlBuilder *dest = (TR::IlBuilder *)destBuilder;
	builder->IfCmpEqual(&dest,
	builder->	Load("value1"),
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

/*************************************************
 * INLINE HELPERS
 *************************************************/

/**
 * generates IL for integer operations. The builder returned will have
 * popped rightObject and leftObject off of the stack and verified that
 * they are both Integer objects.  It will then fetch rightValue and
 * leftValue from these objects.  If either are not Integers then failPath
 * will have sp at the original state.
 */
TR::IlBuilder *
SOMppMethod::generateILForIntergerOps(TR::BytecodeBuilder *builder, TR::IlBuilder **failPath)
{
	builder->Store("rightObject",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			Load("sp"))));

	builder->Store("rightClass",
	builder->	Call("getClass", 1,
	builder->		Load("rightObject")));

	TR::IlBuilder *rightIsInteger = NULL;
	builder->IfThenElse(&rightIsInteger, failPath,
	builder->	EqualTo(
	builder->		Load("rightClass"),
	builder->		ConstInt64((int64_t)integerClass)));

	rightIsInteger->Store("leftObject",
	rightIsInteger->	LoadAt(pInt64,
	rightIsInteger->		ConvertTo(pInt64,
	rightIsInteger->			Sub(
	rightIsInteger->				Load("sp"),
	rightIsInteger->				ConstInt64(8)))));

	rightIsInteger->Store("leftClass",
	rightIsInteger->	Call("getClass", 1,
	rightIsInteger->		Load("leftObject")));

	TR::IlBuilder *isIntegerPath = NULL;
	rightIsInteger->IfThenElse(&isIntegerPath, failPath,
	rightIsInteger->	EqualTo(
	rightIsInteger->		Load("leftClass"),
	rightIsInteger->		ConstInt64((int64_t)integerClass)));

	/* set the stack up so it is at the right place to store the result */
	isIntegerPath->Store("sp",
	isIntegerPath->	Sub(
	isIntegerPath->		Load("sp"),
	isIntegerPath->		ConstInt64(8)));

	/* Read right embedded slot vtable slot + gcField */
	isIntegerPath->Store("rightValueSlot",
	isIntegerPath->	Add(
	isIntegerPath->		Load("rightObject"),
	isIntegerPath->		ConstInt64((int64_t)(sizeof(int64_t)+sizeof(size_t)))));

	/* Read value */
	isIntegerPath->Store("rightValue",
	isIntegerPath->	LoadAt(pInt64,
	isIntegerPath->		ConvertTo(pInt64,
	isIntegerPath->			Load("rightValueSlot"))));

	/* Read left embedded slot vtable slot + gcField */
	isIntegerPath->Store("leftValueSlot",
	isIntegerPath->	Add(
	isIntegerPath->		Load("leftObject"),
	isIntegerPath->		ConstInt64((int64_t)(sizeof(int64_t)+sizeof(size_t)))));

	/* Read value */
	isIntegerPath->Store("leftValue",
	isIntegerPath->	LoadAt(pInt64,
	isIntegerPath->		ConvertTo(pInt64,
	isIntegerPath->			Load("leftValueSlot"))));

	return isIntegerPath;
}

TR::IlBuilder *
SOMppMethod::generateILForIntegerLessThan(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = NULL;
	TR::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	TR::IlBuilder *thenPath = NULL;
	TR::IlBuilder *elsePath = NULL;
	isIntegerPath->IfThenElse(&thenPath, &elsePath,
	isIntegerPath->	LessThan(
	isIntegerPath->		Load("leftValue"),
	isIntegerPath->		Load("rightValue")));

	/* store true result */
	thenPath->StoreAt(pInt64,
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("sp")),
	thenPath->	ConstInt64((int64_t)trueObject));

	/* store false result */
	elsePath->StoreAt(pInt64,
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("sp")),
	elsePath->	ConstInt64((int64_t)falseObject));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForIntegerLessThanEqual(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = NULL;
	TR::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	TR::IlBuilder *thenPath = NULL;
	TR::IlBuilder *elsePath = NULL;
	isIntegerPath->IfThenElse(&thenPath, &elsePath,
	isIntegerPath->	GreaterThan(
	isIntegerPath->		Load("leftValue"),
	isIntegerPath->		Load("rightValue")));

	/* store true result */
	thenPath->StoreAt(pInt64,
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("sp")),
	thenPath->	ConstInt64((int64_t)falseObject));

	/* store false result */
	elsePath->StoreAt(pInt64,
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("sp")),
	elsePath->	ConstInt64((int64_t)trueObject));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForIntegerGreaterThan(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = NULL;
	TR::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	TR::IlBuilder *thenPath = NULL;
	TR::IlBuilder *elsePath = NULL;
	isIntegerPath->IfThenElse(&thenPath, &elsePath,
	isIntegerPath->	GreaterThan(
	isIntegerPath->		Load("leftValue"),
	isIntegerPath->		Load("rightValue")));

	/* store true result */
	thenPath->StoreAt(pInt64,
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("sp")),
	thenPath->	ConstInt64((int64_t)trueObject));

	/* store false result */
	elsePath->StoreAt(pInt64,
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("sp")),
	elsePath->	ConstInt64((int64_t)falseObject));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForIntegerGreaterThanEqual(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = NULL;
	TR::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	TR::IlBuilder *thenPath = NULL;
	TR::IlBuilder *elsePath = NULL;
	isIntegerPath->IfThenElse(&thenPath, &elsePath,
	isIntegerPath->	LessThan(
	isIntegerPath->		Load("leftValue"),
	isIntegerPath->		Load("rightValue")));

	/* store true result */
	thenPath->StoreAt(pInt64,
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("sp")),
	thenPath->	ConstInt64((int64_t)falseObject));

	/* store false result */
	elsePath->StoreAt(pInt64,
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("sp")),
	elsePath->	ConstInt64((int64_t)trueObject));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForIntegerEqual(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = NULL;
	TR::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	TR::IlBuilder *thenPath = NULL;
	TR::IlBuilder *elsePath = NULL;
	isIntegerPath->IfThenElse(&thenPath, &elsePath,
	isIntegerPath->	EqualTo(
	isIntegerPath->		Load("leftValue"),
	isIntegerPath->		Load("rightValue")));

	/* store true result */
	thenPath->StoreAt(pInt64,
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("sp")),
	thenPath->	ConstInt64((int64_t)trueObject));

	/* store false result */
	elsePath->StoreAt(pInt64,
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("sp")),
	elsePath->	ConstInt64((int64_t)falseObject));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForIntegerNotEqual(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = NULL;
	TR::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	TR::IlBuilder *thenPath = NULL;
	TR::IlBuilder *elsePath = NULL;
	isIntegerPath->IfThenElse(&thenPath, &elsePath,
	isIntegerPath->	NotEqualTo(
	isIntegerPath->		Load("leftValue"),
	isIntegerPath->		Load("rightValue")));

	/* store true result */
	thenPath->StoreAt(pInt64,
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("sp")),
	thenPath->	ConstInt64((int64_t)trueObject));

	/* store false result */
	elsePath->StoreAt(pInt64,
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("sp")),
	elsePath->	ConstInt64((int64_t)falseObject));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForIntegerPlus(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = NULL;
	TR::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	isIntegerPath->Store("newInteger",
	isIntegerPath->	Call("newInteger", 1,
	isIntegerPath->		Add(
	isIntegerPath->			Load("leftValue"),
	isIntegerPath->			Load("rightValue"))));

	/* store new integer */
	isIntegerPath->StoreAt(pInt64,
	isIntegerPath->	ConvertTo(pInt64,
	isIntegerPath->		Load("sp")),
	isIntegerPath->	Load("newInteger"));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForIntegerMinus(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = NULL;
	TR::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	isIntegerPath->Store("newInteger",
	isIntegerPath->	Call("newInteger", 1,
	isIntegerPath->		Sub(
	isIntegerPath->			Load("leftValue"),
	isIntegerPath->			Load("rightValue"))));

	/* store new integer */
	isIntegerPath->StoreAt(pInt64,
	isIntegerPath->	ConvertTo(pInt64,
	isIntegerPath->		Load("sp")),
	isIntegerPath->	Load("newInteger"));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForIntegerStar(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = NULL;
	TR::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	isIntegerPath->Store("newInteger",
	isIntegerPath->	Call("newInteger", 1,
	isIntegerPath->		Mul(
	isIntegerPath->			Load("leftValue"),
	isIntegerPath->			Load("rightValue"))));

	/* store new integer */
	isIntegerPath->StoreAt(pInt64,
	isIntegerPath->	ConvertTo(pInt64,
	isIntegerPath->		Load("sp")),
	isIntegerPath->	Load("newInteger"));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForIntegerValue(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = NULL;

	builder->Store("integerObject",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			Load("sp"))));

	builder->Store("integerClass",
	builder->	Call("getClass", 1,
	builder->		Load("integerObject")));

	TR::IlBuilder *isInteger = NULL;

	builder->IfThenElse(&isInteger, &failInline,
	builder->	EqualTo(
	builder->		Load("integerClass"),
	builder->		ConstInt64((int64_t)integerClass)));

	/* store new integer */
	isInteger->StoreAt(pInt64,
	isInteger->	ConvertTo(pInt64,
	isInteger->		Load("sp")),
	isInteger->	Load("integerObject"));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForIntegerMax(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = NULL;
	TR::IlBuilder *isIntegerPath = generateILForIntergerOps(builder, &failInline);

	TR::IlBuilder *thenPath = NULL;
	TR::IlBuilder *elsePath = NULL;
	isIntegerPath->IfThenElse(&thenPath, &elsePath,
	isIntegerPath->	LessThan(
	isIntegerPath->		Load("leftValue"),
	isIntegerPath->		Load("rightValue")));

	/* store true result */
	thenPath->StoreAt(pInt64,
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("sp")),
	thenPath->	Load("rightObject"));

	/* store false result */
	elsePath->StoreAt(pInt64,
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("sp")),
	elsePath->	Load("leftObject"));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForIntegerNegated(TR::BytecodeBuilder *builder)
{
	builder->Store("integerObject",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			Load("sp"))));

	builder->Store("integerClass",
	builder->	Call("getClass", 1,
	builder->		Load("integerObject")));

	TR::IlBuilder *isInteger = NULL;
	TR::IlBuilder *failInline = NULL;

	builder->IfThenElse(&isInteger, &failInline,
	builder->	EqualTo(
	builder->		Load("integerClass"),
	builder->		ConstInt64((int64_t)integerClass)));

	/* Read right embedded slot vtable slot + gcField */
	isInteger->Store("valueSlot",
	isInteger->	Add(
	isInteger->		Load("integerObject"),
	isInteger->		ConstInt64((int64_t)(sizeof(int64_t)+sizeof(size_t)))));

	/* Read value */
	isInteger->Store("integerValue",
	isInteger->	LoadAt(pInt64,
	isInteger->		ConvertTo(pInt64,
	isInteger->			Load("valueSlot"))));

	isInteger->Store("negatedValue",
	isInteger->	Call("newInteger", 1,
	isInteger->		Sub(
	isInteger->			ConstInt64(0),
	isInteger->			Load("integerValue"))));

	/* store new integer */
	isInteger->StoreAt(pInt64,
	isInteger->	ConvertTo(pInt64,
	isInteger->		Load("sp")),
	isInteger->	Load("negatedValue"));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForArrayAt(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = NULL;

	builder->Store("indexObject",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			Load("sp"))));

	builder->Store("indexClass",
	builder->	Call("getClass", 1,
	builder->		Load("indexObject")));

	TR::IlBuilder *indexIsInteger = NULL;

	builder->IfThenElse(&indexIsInteger, &failInline,
	builder->	EqualTo(
	builder->		Load("indexClass"),
	builder->		ConstInt64((int64_t)integerClass)));

	indexIsInteger->Store("arrayObject",
	indexIsInteger->	LoadAt(pInt64,
	indexIsInteger->		ConvertTo(pInt64,
	indexIsInteger->			Add(
	indexIsInteger->				Load("sp"),
	indexIsInteger->				ConstInt64(-8)))));

	indexIsInteger->Store("arrayClass",
	indexIsInteger->	Call("getClass", 1,
	indexIsInteger->		Load("arrayObject")));

	TR::IlBuilder *isArrayPath = NULL;

	indexIsInteger->IfThenElse(&isArrayPath, &failInline,
	indexIsInteger->	EqualTo(
	indexIsInteger->		Load("arrayClass"),
	indexIsInteger->		ConstInt64((int64_t)arrayClass)));

	/* Read index embedded slot vtable slot + gcField */
	isArrayPath->Store("indexValueSlot",
	isArrayPath->	Add(
	isArrayPath->		Load("indexObject"),
	isArrayPath->		ConstInt64((int64_t)(sizeof(int64_t)+sizeof(size_t)))));

	/* Read index value */
	isArrayPath->Store("indexValue",
	isArrayPath->	LoadAt(pInt64,
	isArrayPath->		ConvertTo(pInt64,
	isArrayPath->			Load("indexValueSlot"))));

	/* fetch array field */
	isArrayPath->Store("result",
	isArrayPath->	Call("getIndexableField", 2,
	isArrayPath->		Load("arrayObject"),
	isArrayPath->		Load("indexValue")));

	/* decrement the stack to hold result */
	/* this method had 2 args on the stack so it only has to pop 1 arg to store result */
	isArrayPath->Store("sp",
	isArrayPath->	Add(
	isArrayPath->		Load("sp"),
	isArrayPath->		ConstInt64(-8)));

	/* store new integer */
	isArrayPath->StoreAt(pInt64,
	isArrayPath->	ConvertTo(pInt64,
	isArrayPath->		Load("sp")),
	isArrayPath->	Load("result"));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForArrayAtPut(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = NULL;

	builder->Store("valueObject",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			Load("sp"))));

	builder->Store("indexObject",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			Add(
	builder->				Load("sp"),
	builder->				ConstInt64(-8)))));

	builder->Store("indexClass",
	builder->	Call("getClass", 1,
	builder->		Load("indexObject")));

	TR::IlBuilder *indexIsInteger = NULL;

	builder->IfThenElse(&indexIsInteger, &failInline,
	builder->	EqualTo(
	builder->		Load("indexClass"),
	builder->		ConstInt64((int64_t)integerClass)));

	/* peek the array object without popping */
	indexIsInteger->Store("arrayObject",
	indexIsInteger->	LoadAt(pInt64,
	indexIsInteger->		ConvertTo(pInt64,
	indexIsInteger->			Add(
	indexIsInteger->				Load("sp"),
	indexIsInteger->				ConstInt64(-16)))));

	indexIsInteger->Store("arrayClass",
	indexIsInteger->	Call("getClass", 1,
	indexIsInteger->		Load("arrayObject")));

	TR::IlBuilder *isArrayPath = NULL;

	indexIsInteger->IfThenElse(&isArrayPath, &failInline,
	indexIsInteger->	EqualTo(
	indexIsInteger->		Load("arrayClass"),
	indexIsInteger->		ConstInt64((int64_t)arrayClass)));

	isArrayPath->Store("sp",
	isArrayPath->	Add(
	isArrayPath->		Load("sp"),
	isArrayPath->		ConstInt64(-16)));

	/* Read index embedded slot vtable slot + gcField */
	isArrayPath->Store("indexValueSlot",
	isArrayPath->	Add(
	isArrayPath->		Load("indexObject"),
	isArrayPath->		ConstInt64((int64_t)(sizeof(int64_t)+sizeof(size_t)))));

	/* Read index value */
	isArrayPath->Store("indexValue",
	isArrayPath->	LoadAt(pInt64,
	isArrayPath->		ConvertTo(pInt64,
	isArrayPath->			Load("indexValueSlot"))));

	/* set array field */
	isArrayPath->Call("setIndexableField", 3,
	isArrayPath->	Load("arrayObject"),
	isArrayPath->	Load("indexValue"),
	isArrayPath->	Load("valueObject"));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForArrayLength(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *failInline = NULL;

	builder->Store("arrayObject",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			Load("sp"))));

	builder->Store("arrayClass",
	builder->	Call("getClass", 1,
	builder->		Load("arrayObject")));

	TR::IlBuilder *isArrayPath = NULL;

	builder->IfThenElse(&isArrayPath, &failInline,
	builder->	EqualTo(
	builder->		Load("arrayClass"),
	builder->		ConstInt64((int64_t)arrayClass)));

	/* fetch array field */
	isArrayPath->Store("result",
	isArrayPath->	Call("getNumberOfIndexableFields", 1,
	isArrayPath->		Load("arrayObject")));

	/* store new integer */
	isArrayPath->StoreAt(pInt64,
	isArrayPath->	ConvertTo(pInt64,
	isArrayPath->		Load("sp")),
	isArrayPath->	Load("result"));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForNilisNil(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *isNil = NULL;
	TR::IlBuilder *notNil = NULL;

	builder->Store("currentObject",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			Load("sp"))));

	builder->Store("nilObject",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			ConstInt64((int64_t)&nilObject))));

	builder->IfThenElse(&isNil, &notNil,
	builder->	EqualTo(
	builder->		Load("currentObject"),
	builder->		Load("nilObject")));

	isNil->StoreAt(pInt64,
	isNil->	ConvertTo(pInt64,
	isNil->		Load("sp")),
	isNil-> LoadAt(pInt64,
	isNil->		ConvertTo(pInt64,
	isNil->			ConstInt64((int64_t)&trueObject))));

	notNil->StoreAt(pInt64,
	notNil->	ConvertTo(pInt64,
	notNil->		Load("sp")),
	notNil-> LoadAt(pInt64,
	notNil->		ConvertTo(pInt64,
	notNil->			ConstInt64((int64_t)&falseObject))));

	return nullptr;
}

TR::IlBuilder *
SOMppMethod::generateILForBooleanNot(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *isTrue = NULL;
	TR::IlBuilder *notTrue = NULL;

	builder->Store("currentObject",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			Load("sp"))));

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
	isTrue->	ConvertTo(pInt64,
	isTrue->		Load("sp")),
	isTrue->	Load("falseObject"));

	TR::IlBuilder *isFalse = NULL;
	TR::IlBuilder *failInline = NULL;

	notTrue->IfThenElse(&isFalse, &failInline,
	notTrue->	EqualTo(
	notTrue->		Load("currentObject"),
	notTrue->		Load("falseObject")));

	isFalse->StoreAt(pInt64,
	isFalse->	ConvertTo(pInt64,
	isFalse->		Load("sp")),
	isFalse->	Load("trueObject"));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForBooleanAnd(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *isFalse = NULL;
	TR::IlBuilder *failInline = NULL;

	builder->Store("newsp",
	builder->	Sub(
	builder->		Load("sp"),
	builder->		ConstInt64(8)));

	builder->Store("currentObject",
	builder->	LoadAt(pInt64,
	builder->		Load("newsp")));

	builder->Store("falseObject",
	builder->	LoadAt(pInt64,
	builder->		ConstInt64((int64_t)&falseObject)));

	builder->IfThenElse(&isFalse, &failInline,
	builder->	EqualTo(
	builder->		Load("currentObject"),
	builder->		Load("falseObject")));

	isFalse->Store("sp",
	isFalse->	Load("newsp"));

	isFalse->StoreAt(pInt64,
	isFalse->	Load("sp"),
	isFalse->	Load("falseObject"));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForBooleanOr(TR::BytecodeBuilder *builder)
{
	TR::IlBuilder *isTrue = NULL;
	TR::IlBuilder *failInline = NULL;

	builder->Store("newsp",
	builder->	Sub(
	builder->		Load("sp"),
	builder->		ConstInt64(8)));

	builder->Store("currentObject",
	builder->	LoadAt(pInt64,
	builder->		Load("newsp")));

	builder->Store("trueObject",
	builder->	LoadAt(pInt64,
	builder->		ConstInt64((int64_t)&trueObject)));

	builder->IfThenElse(&isTrue, &failInline,
	builder->	EqualTo(
	builder->		Load("currentObject"),
	builder->		Load("trueObject")));

	isTrue->Store("sp",
	isTrue->	Load("newsp"));

	isTrue->StoreAt(pInt64,
	isTrue->	Load("sp"),
	isTrue->	Load("trueObject"));

	return failInline;
}

TR::IlBuilder *
SOMppMethod::generateILForDoubleOps(TR::BytecodeBuilder *builder, TR::IlBuilder **failPath)
{
	builder->Store("rightObject",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			Load("sp"))));

	builder->Store("rightClass",
	builder->	Call("getClass", 1,
	builder->		Load("rightObject")));

	TR::IlBuilder *rightIsDouble = NULL;
	builder->IfThenElse(&rightIsDouble, failPath,
	builder->	EqualTo(
	builder->		Load("rightClass"),
	builder->		ConstInt64((int64_t)doubleClass)));

	rightIsDouble->Store("leftObject",
	rightIsDouble->	LoadAt(pInt64,
	rightIsDouble->		ConvertTo(pInt64,
	rightIsDouble->			Sub(
	rightIsDouble->				Load("sp"),
	rightIsDouble->				ConstInt64(8)))));

	rightIsDouble->Store("leftClass",
	rightIsDouble->	Call("getClass", 1,
	rightIsDouble->		Load("leftObject")));

	TR::IlBuilder *isDoublePath = NULL;
	rightIsDouble->IfThenElse(&isDoublePath, failPath,
	rightIsDouble->	EqualTo(
	rightIsDouble->		Load("leftClass"),
	rightIsDouble->		ConstInt64((int64_t)doubleClass)));

	/* set the stack up so it is at the right place to store the result */
	isDoublePath->Store("sp",
	isDoublePath->	Sub(
	isDoublePath->		Load("sp"),
	isDoublePath->		ConstInt64(8)));

	/* Read right embedded slot vtable slot + gcField */
	isDoublePath->Store("rightValueSlot",
	isDoublePath->	Add(
	isDoublePath->		Load("rightObject"),
	isDoublePath->		ConstInt64((int64_t)(sizeof(int64_t)+sizeof(size_t)))));

	/* Read value */
	isDoublePath->Store("rightValue",
	isDoublePath->	LoadAt(pInt64,
	isDoublePath->		ConvertTo(pInt64,
	isDoublePath->			Load("rightValueSlot"))));

	/* Read left embedded slot vtable slot + gcField */
	isDoublePath->Store("leftValueSlot",
	isDoublePath->	Add(
	isDoublePath->		Load("leftObject"),
	isDoublePath->		ConstInt64((int64_t)(sizeof(int64_t)+sizeof(size_t)))));

	/* Read value */
	isDoublePath->Store("leftValue",
	isDoublePath->	LoadAt(pInt64,
	isDoublePath->		ConvertTo(pInt64,
	isDoublePath->			Load("leftValueSlot"))));

	return isDoublePath;
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
	isDoublePath->		ConvertTo(Double,
	isDoublePath->			Load("leftValue")),
	isDoublePath->		ConvertTo(Double,
	isDoublePath->			Load("rightValue"))));

	/* store true result */
	thenPath->StoreAt(pInt64,
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("sp")),
	thenPath->	ConstInt64((int64_t)falseObject));

	/* store false result */
	elsePath->StoreAt(pInt64,
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("sp")),
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
	isDoublePath->		ConvertTo(Double,
	isDoublePath->			Load("leftValue")),
	isDoublePath->		ConvertTo(Double,
	isDoublePath->			Load("rightValue"))));

	/* store true result */
	thenPath->StoreAt(pInt64,
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("sp")),
	thenPath->	ConstInt64((int64_t)trueObject));

	/* store false result */
	elsePath->StoreAt(pInt64,
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("sp")),
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
	isDoublePath->		ConvertTo(Double,
	isDoublePath->			Load("leftValue")),
	isDoublePath->		ConvertTo(Double,
	isDoublePath->			Load("rightValue"))));

	/* store true result */
	thenPath->StoreAt(pInt64,
	thenPath->	ConvertTo(pInt64,
	thenPath->		Load("sp")),
	thenPath->	ConstInt64((int64_t)falseObject));

	/* store false result */
	elsePath->StoreAt(pInt64,
	elsePath->	ConvertTo(pInt64,
	elsePath->		Load("sp")),
	elsePath->	ConstInt64((int64_t)trueObject));

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
		} else if (0 == strcmp("max:", signatureChars)) {
			return generateILForIntegerMax(builder);
		} else if (0 == strcmp("negated", signatureChars)){
			return generateILForIntegerNegated(builder);
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
		} else if ((0 == strcmp("or:", signatureChars)) && false) {
			/* TODO do not gen this for now as they cause more of a perf issue than win */
			return generateILForBooleanOr(builder);
		} else if ((0 == strcmp("and:", signatureChars)) && false) {
			/* TODO do not gen this for now as they cause more of a perf issue than win */
			return generateILForBooleanAnd(builder);
		}
	} else if ((VMClass*)doubleClass == receiverFromCache) {
		if (0 == strcmp("<=", signatureChars)) {
			return generateILForDoubleLessThanEqual(builder);
		} else if (0 == strcmp(">", signatureChars)) {
			return generateILForDoubleGreaterThan(builder);
		} else if (0 == strcmp(">=", signatureChars)) {
			return generateILForDoubleGreaterThanEqual(builder);
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

			/* pop all arguments off of the stack */
			inlineBuilder->Store("sp",
			inlineBuilder->	Add(
			inlineBuilder->		Load("receiverAddress"),
			inlineBuilder->		ConstInt64(-8)));

			long bytecodeCount = vmMethod->GetNumberOfBytecodes();
			long bytecodeIndex = 0;
			while (bytecodeIndex < bytecodeCount) {
				uint8_t bc = vmMethod->GetBytecode(bytecodeIndex);

				switch(bc) {
				/* TODO call helpers that share an implementation with the general bytecode generation */
				case BC_DUP:
				{
					inlineBuilder->Store("value",
					inlineBuilder->	LoadAt(pInt64,
					inlineBuilder->		ConvertTo(pInt64,
					inlineBuilder->			Load("sp"))));

					/* increment stack */
					inlineBuilder->Store("sp",
					inlineBuilder->	Add(
					inlineBuilder->		Load("sp"),
					inlineBuilder->		ConstInt64(8)));

					/* write to stack */
					inlineBuilder->StoreAt(pInt64,
					inlineBuilder->	ConvertTo(pInt64,
					inlineBuilder->		Load("sp")),
					inlineBuilder->	Load("value"));
					break;
				}
				case BC_PUSH_FIELD:
				{
					uint8_t fieldIndex = vmMethod->GetBytecode(bytecodeIndex + 1);

					inlineBuilder->Store("fieldValue",
					inlineBuilder->	Call("getFieldFrom", 2,
					inlineBuilder->		Load("receiverObject"),
					inlineBuilder->		ConstInt64((int64_t)fieldIndex)));

					inlineBuilder->Store("sp",
					inlineBuilder->	Add(
					inlineBuilder->		Load("sp"),
					inlineBuilder->		ConstInt64((int64_t)8)));

					inlineBuilder->StoreAt(pInt64,
					inlineBuilder->	ConvertTo(pInt64,
					inlineBuilder->		Load("sp")),
					inlineBuilder->	Load("fieldValue"));

					break;
				}
				case BC_PUSH_CONSTANT:
				{
					uint8_t valueOffset = vmMethod->GetBytecode(bytecodeIndex + 1);

					inlineBuilder->Store("constant",
					inlineBuilder->	LoadAt(pInt64,
					inlineBuilder->		IndexAt(pInt64,
					inlineBuilder->			ConvertTo(pInt64,
					inlineBuilder->				ConstInt64((int64_t)vmMethod->indexableFields)),
					inlineBuilder->			ConstInt64(valueOffset))));

					/* increment stack */
					inlineBuilder->Store("sp",
					inlineBuilder->	Add(
					inlineBuilder->		Load("sp"),
					inlineBuilder->		ConstInt64(8)));

					/* write constant to stack */
					inlineBuilder->StoreAt(pInt64,
					inlineBuilder->	ConvertTo(pInt64,
					inlineBuilder->		Load("sp")),
					inlineBuilder->	Load("constant"));

					break;
				}
				case BC_PUSH_GLOBAL:
				{
					VMSymbol* globalName = static_cast<VMSymbol*>(vmMethod->GetConstant(bytecodeIndex));

					inlineBuilder->Store("global",
					inlineBuilder->	Call("getGlobal", 1,
					inlineBuilder->		ConstInt64((int64_t)globalName)));

					inlineBuilder->Store("sp",
					inlineBuilder->	Add(
					inlineBuilder->		Load("sp"),
					inlineBuilder->		ConstInt64((int64_t)8)));

					inlineBuilder->StoreAt(pInt64,
					inlineBuilder->	ConvertTo(pInt64,
					inlineBuilder->		Load("sp")),
					inlineBuilder->	Load("global"));

					break;
				}
				case BC_PUSH_ARGUMENT:
				{
					uint8_t argumentIndex = vmMethod->GetBytecode(bytecodeIndex + 1);

					inlineBuilder->Store("sp",
					inlineBuilder->	Add(
					inlineBuilder->		Load("sp"),
					inlineBuilder->		ConstInt64((int64_t)8)));

					inlineBuilder->StoreAt(pInt64,
					inlineBuilder->	ConvertTo(pInt64,
					inlineBuilder->		Load("sp")),
					inlineBuilder-> LoadAt(pInt64,
					inlineBuilder->		IndexAt(pInt64,
					inlineBuilder->			Load("argumentsArray"),
					inlineBuilder->			ConstInt32(argumentIndex))));

					break;
				}
				case BC_POP:
				{
					inlineBuilder->Store("sp",
					inlineBuilder->	Add(
					inlineBuilder->		Load("sp"),
					inlineBuilder->		ConstInt64(-8)));
					break;
				}
				case BC_POP_FIELD:
				{
					uint8_t fieldIndex = vmMethod->GetBytecode(bytecodeIndex + 1);

					inlineBuilder->Store("value",
					inlineBuilder->	LoadAt(pInt64,
					inlineBuilder->		ConvertTo(pInt64,
					inlineBuilder->			Load("sp"))));

					inlineBuilder->Store("sp",
					inlineBuilder->	Add(
					inlineBuilder->		Load("sp"),
					inlineBuilder->		ConstInt64(-8)));

					inlineBuilder->Call("setFieldTo", 3,
					inlineBuilder->	Load("receiverObject"),
					inlineBuilder->	ConstInt64((int64_t)fieldIndex),
					inlineBuilder->	Load("value"));

					break;
				}
				case BC_RETURN_LOCAL:
				{
					inlineBuilder->Store("returnValue",
					inlineBuilder->	LoadAt(pInt64,
					inlineBuilder->		ConvertTo(pInt64,
					inlineBuilder->			Load("sp"))));

					inlineBuilder->Store("sp",
					inlineBuilder->	Load("receiverAddress"));

					inlineBuilder->StoreAt(pInt64,
					inlineBuilder->	ConvertTo(pInt64,
					inlineBuilder->		Load("sp")),
					inlineBuilder->	Load("returnValue"));

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
	while (i < bytecodeCount) {
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
				return false;
			}
			break;
		}
		default:
			return false;
		}
		i += Bytecode::GetBytecodeLength(bc);
	}
	return true;
}
