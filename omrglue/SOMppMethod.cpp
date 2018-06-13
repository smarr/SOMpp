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
#include "vmobjects/VMBlock.h"
#include "vmobjects/VMFrame.h"
#include "vmobjects/VMMethod.h"
#include "vmobjects/VMSymbol.h"
#include "interpreter/bytecodes.h"
#include "interpreter/Interpreter.h"

#include "BytecodeHelper.hpp"

#define DO_DEBUG_PRINTS 1

#if DEBUG
#ifndef UNITTESTS
#define SOM_METHOD_DEBUG true
#endif
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
      _stack(nullptr),
      _stackTop(nullptr)
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

#define STACK(b)	      (((SOMppVMState *)(b)->vmState())->_stack)
#define COMMIT(b)         ((b)->vmState()->Commit(b))
#define RELOAD(b)         ((b)->vmState()->Reload(b))
#define PUSH(b,v)	      (STACK(b)->Push(b,v))
#define POP(b)            (STACK(b)->Pop(b))
#define TOP(b)            (STACK(b)->Top())
#define DUP(b)            (STACK(b)->Dup(b))
#define DROP(b,d)         (STACK(b)->Drop(b,d))
#define DROPALL(b)        (STACK(b)->DropAll(b))
#define PICK(b,d)         (STACK(b)->Pick(d))
#define GET_STACKTOP(b)   (STACK(b)->GetStackTop())
#define SET_STACKTOP(b,v) (STACK(b)->SetStackTop(v))

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

TR::IlValue *
SOMppMethod::add(TR::BytecodeBuilder *builder, TR::IlValue *param1, TR::IlValue *param2)
{
	return builder->Add(param1, param2);
}
TR::IlValue *
SOMppMethod::sub(TR::BytecodeBuilder *builder, TR::IlValue *param1, TR::IlValue *param2)
{
	return builder->Sub(param1, param2);
}
TR::IlValue *
SOMppMethod::mul(TR::BytecodeBuilder *builder, TR::IlValue *param1, TR::IlValue *param2)
{
	return builder->Mul(param1, param2);
}
TR::IlValue *
SOMppMethod::div(TR::BytecodeBuilder *builder, TR::IlValue *param1, TR::IlValue *param2)
{
	return builder->Div(param1, param2);
}
TR::IlValue *
SOMppMethod::percent(TR::BytecodeBuilder *builder, TR::IlValue *param1, TR::IlValue *param2)
{
	TR::IlValue *divResult = builder->Div(param1, param2);
	TR::IlValue *mulResult = builder->Mul(divResult, param2);

	builder->Store("subResult",
	builder->	Sub(param1, mulResult));

	TR::IlBuilder *greater = nullptr;
	builder->IfThen(&greater,
	builder->	GreaterThan(param1,
	builder->		ConstInt64(0)));

	TR::IlBuilder *lessThan = nullptr;
	greater->IfThen(&lessThan,
	greater->	LessThan(param1,
	greater->		ConstInt64(0)));

	lessThan->Store("subResult",
	lessThan->	Add(
	lessThan->		Load("subResult"), param2));

	return builder->Load("subResult");
}
TR::IlValue *
SOMppMethod::andVals(TR::BytecodeBuilder *builder, TR::IlValue *param1, TR::IlValue *param2)
{
	return builder->And(param1, param2);
}
TR::IlValue *
SOMppMethod::lessThan(TR::BytecodeBuilder *builder, TR::IlValue *param1, TR::IlValue *param2)
{
	return builder->LessThan(param1, param2);
}
TR::IlValue *
SOMppMethod::greaterThan(TR::BytecodeBuilder *builder, TR::IlValue *param1, TR::IlValue *param2)
{
	return builder->GreaterThan(param1, param2);
}
TR::IlValue *
SOMppMethod::equalTo(TR::BytecodeBuilder *builder, TR::IlValue *param1, TR::IlValue *param2)
{
	return builder->EqualTo(param1, param2);
}
void
SOMppMethod::forLoopUp(TR::BytecodeBuilder *builder, const char *index, TR::IlBuilder **loop, TR::IlValue *start, TR::IlValue *end, TR::IlValue *increment)
{
	builder->ForLoopUp(index, loop, start, end, increment);
}
void
SOMppMethod::forLoopDown(TR::BytecodeBuilder *builder, const char *index, TR::IlBuilder **loop, TR::IlValue *start, TR::IlValue *end, TR::IlValue *increment)
{
	builder->ForLoopDown(index, loop, start, end, increment);
}

SOMppMethod::SOMppMethod(TR::TypeDictionary *types, VMMethod *vmMethod, bool inlineCalls) :
		MethodBuilder(types),
		method(vmMethod),
		stackTopForErrorHandling(-1),
		doInlining(inlineCalls)
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
	DefineLocal("frameOuterContext", Int64);
	DefineLocal("frameArguments", Int64);
	DefineLocal("frameLocals", Int64);
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
	DefineFunction((char *)"getSuperClass", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::GET_SUPER_CLASS_LINE, (void *)&BytecodeHelper::getSuperClass, Int64, 1, Int64);
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
SOMppMethod::calculateBytecodeIndexForJump(VMMethod *vmMethod, long bytecodeIndex)
{
	int64_t target = 0;
    target |= vmMethod->GetBytecode(bytecodeIndex + 1);
    target |= vmMethod->GetBytecode(bytecodeIndex + 2) << 8;
    target |= vmMethod->GetBytecode(bytecodeIndex + 3) << 16;
    target |= vmMethod->GetBytecode(bytecodeIndex + 4) << 24;

    return target;
}

TR::BytecodeBuilder **
SOMppMethod::createBytecodesBuilder(VMMethod *vmMethod)
{
	long numOfBytecodes = vmMethod->GetNumberOfBytecodes();
	long tableSize = sizeof(TR::BytecodeBuilder *) * numOfBytecodes;
	TR::BytecodeBuilder **bytecodeBuilderTable = (TR::BytecodeBuilder **)malloc(tableSize);
	if (nullptr != bytecodeBuilderTable) {

		memset(bytecodeBuilderTable, 0, tableSize);

		long i = 0;
		while (i < numOfBytecodes) {
			uint8_t bc = vmMethod->GetBytecode(i);
			bytecodeBuilderTable[i] = OrphanBytecodeBuilder(i, Bytecode::GetBytecodeName(bc));
			i += Bytecode::GetBytecodeLength(bc);
		}
	}
	return bytecodeBuilderTable;
}

void
SOMppMethod::justReturn(TR::IlBuilder *from)
{
	from->Return();
}

bool
SOMppMethod::buildIL()
{
	bool canHandle = true;
	long numOfBytecodes = method->GetNumberOfBytecodes();
	if (numOfBytecodes < 2) {
		return false;
	}

	TR::BytecodeBuilder **bytecodeBuilderTable = createBytecodesBuilder(method);
	if (nullptr == bytecodeBuilderTable) {
		return false;
	}

#if SOM_METHOD_DEBUG
	fprintf(stderr, "\nGenerating code for %s\n", methodName);
#endif

	stackTop = new OMR::VirtualMachineRegisterInStruct(this, "VMFrame", "frame", "stack_ptr", "SP");
	stack = new OMR::VirtualMachineOperandStack(this, 32, valueType, stackTop);

	SOMppVMState *vmState = new SOMppVMState(stack, stackTop);
	setVMState(vmState);

	Store("frameOuterContext", getOuterContext(this));
	Store("frameArguments",
		LoadIndirect("VMFrame", "arguments",
			Load("frame")));
	Store("frameLocals",
		LoadIndirect("VMFrame", "locals",
			Load("frame")));

	Store("frameContext",
		LoadIndirect("VMFrame", "context",
			Load("frame")));
	Store("frameContextArguments",
		LoadIndirect("VMFrame", "arguments",
			Load("frameContext")));
	Store("frameContextLocals",
		LoadIndirect("VMFrame", "locals",
			Load("frameContext")));

	AppendBuilder(bytecodeBuilderTable[0]);

	canHandle = generateILForBytecodes(method, bytecodeBuilderTable);

	free((void *)bytecodeBuilderTable);
	return canHandle;
}

bool SOMppMethod::generateILForBytecodes(VMMethod *vmMethod, TR::BytecodeBuilder **bytecodeBuilderTable)
{
	long numOfBytecodes = method->GetNumberOfBytecodes();
	int32_t bytecodeIndex = GetNextBytecodeFromWorklist();
	bool canHandle = true;

	while (canHandle && (-1 != bytecodeIndex)) {
		uint8_t bytecode = vmMethod->GetBytecode(bytecodeIndex);
		int32_t nextBCIndex = bytecodeIndex + Bytecode::GetBytecodeLength(bytecode);
		TR::BytecodeBuilder *builder = bytecodeBuilderTable[bytecodeIndex];
		TR::BytecodeBuilder *fallThroughBuilder = nullptr;

		if (nextBCIndex < numOfBytecodes) {
			fallThroughBuilder = bytecodeBuilderTable[nextBCIndex];
		}

#if SOM_METHOD_DEBUG
		fprintf(stderr, "\tbytcode %s index %d started ... builder %lx ", Bytecode::GetBytecodeName(bytecode), bytecodeIndex, (int64_t)builder);
#endif
		switch(bytecode) {
		case BC_HALT:
			canHandle = false;
			break;
		case BC_DUP:
			doDup(builder);
			builder->AddFallThroughBuilder(fallThroughBuilder);
			break;
		case BC_PUSH_LOCAL:
			doPushLocal(builder, bytecodeIndex);
			builder->AddFallThroughBuilder(fallThroughBuilder);
			break;
		case BC_PUSH_ARGUMENT:
			doPushArgument(builder, bytecodeIndex);
			builder->AddFallThroughBuilder(fallThroughBuilder);
			break;
		case BC_PUSH_FIELD:
			doPushField(builder, bytecodeIndex);
			builder->AddFallThroughBuilder(fallThroughBuilder);
			break;
		case BC_PUSH_BLOCK:
			doPushBlock(builder, bytecodeIndex);
			builder->AddFallThroughBuilder(fallThroughBuilder);
			break;
		case BC_PUSH_CONSTANT:
			doPushConstant(builder, bytecodeIndex);
			builder->AddFallThroughBuilder(fallThroughBuilder);
			break;
		case BC_PUSH_GLOBAL:
			doPushGlobal(builder, bytecodeIndex);
			builder->AddFallThroughBuilder(fallThroughBuilder);
			break;
		case BC_POP:
			doPop(builder);
			builder->AddFallThroughBuilder(fallThroughBuilder);
			break;
		case BC_POP_LOCAL:
			doPopLocal(builder, bytecodeIndex);
			builder->AddFallThroughBuilder(fallThroughBuilder);
			break;
		case BC_POP_ARGUMENT:
			doPopArgument(builder, bytecodeIndex);
			builder->AddFallThroughBuilder(fallThroughBuilder);
			break;
		case BC_POP_FIELD:
			doPopField(builder, bytecodeIndex);
			builder->AddFallThroughBuilder(fallThroughBuilder);
			break;
		case BC_SEND:
			doSend(builder, bytecodeBuilderTable, bytecodeIndex, fallThroughBuilder);
			/* this bytecode will add the fall through builder inside */
			break;
		case BC_SUPER_SEND:
			doSuperSend(builder, bytecodeBuilderTable, bytecodeIndex);
			builder->AddFallThroughBuilder(fallThroughBuilder);
			break;
		case BC_RETURN_LOCAL:
			doReturnLocal(builder, bytecodeIndex);
			break;
		case BC_RETURN_NON_LOCAL:
			doReturnNonLocal(builder, bytecodeIndex);
			break;
		case BC_JUMP_IF_FALSE:
			doJumpIfFalse(builder, bytecodeBuilderTable, bytecodeIndex);
			builder->AddFallThroughBuilder(fallThroughBuilder);
			break;
		case BC_JUMP_IF_TRUE:
			doJumpIfTrue(builder, bytecodeBuilderTable, bytecodeIndex);
			builder->AddFallThroughBuilder(fallThroughBuilder);
			break;
		case BC_JUMP:
			doJump(builder, bytecodeBuilderTable, bytecodeIndex);
			break;
		default:
			canHandle = false;
		}

#if SOM_METHOD_DEBUG
		fprintf(stderr, "\tfinished\n");
#endif
		bytecodeIndex = GetNextBytecodeFromWorklist();
	}

	return canHandle;
}

void
SOMppMethod::doDup(TR::BytecodeBuilder *builder)
{
	DUP(builder);
}

void
SOMppMethod::doPushLocal(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	uint8_t index = method->GetBytecode(bytecodeIndex + 1);
	uint8_t level = method->GetBytecode(bytecodeIndex + 2);
#if SOM_METHOD_DEBUG
	fprintf(stderr, " %d %d ", index, level);
#endif
	TR::IlValue *locals = nullptr;
	if (level == 0) {
		locals = builder->Load("frameLocals");
	} else {
		const char *contextName = getContext(builder, level);
		locals =
		builder->LoadIndirect("VMFrame", "locals",
		builder->	Load(contextName));
	}

	pushValueFromArray(builder, locals, index);
}

void
SOMppMethod::doPushArgument(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	uint8_t index = method->GetBytecode(bytecodeIndex + 1);
	uint8_t level = method->GetBytecode(bytecodeIndex + 2);
#if SOM_METHOD_DEBUG
	fprintf(stderr, " %d %d ", index, level);
#endif
	TR::IlValue *arguments = nullptr;
	if (0 == level) {
		arguments = builder->Load("frameArguments");
	} else {
		const char *contextName = getContext(builder, level);
		arguments =
		builder->LoadIndirect("VMFrame", "arguments",
		builder->	Load(contextName));
	}

	pushValueFromArray(builder, arguments, index);
}

void
SOMppMethod::doPushField(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	TR::IlValue *object = getSelf(builder);
#if SOM_METHOD_DEBUG
	fprintf(stderr, " %d ", method->GetBytecode(bytecodeIndex + 1));
#endif
	pushField(builder, object, method->GetBytecode(bytecodeIndex + 1));
}

void
SOMppMethod::doPushBlock(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	/* TODO come back and handle optimization in Interpreter::doPushBlock */
	VMMethod *blockMethod = static_cast<VMMethod*>(method->GetConstant(bytecodeIndex));
	long numOfArgs = blockMethod->GetNumberOfArguments();

	blockMethods.push(blockMethod);
#if SOM_METHOD_DEBUG
	fprintf(stderr, " %p ", blockMethod);
#endif

	TR::IlValue *block =
	builder->Call("getNewBlock", 3,
	builder->	Load("frame"),
	builder->	ConstInt64((int64_t)blockMethod),
	builder->	ConstInt64((int64_t)numOfArgs));

	PUSH(builder, block);
}

void
SOMppMethod::doPushConstant(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
#if SOM_METHOD_DEBUG
	fprintf(stderr, " %d ", method->GetBytecode(bytecodeIndex + 1));
#endif
	pushConstant(builder, method, method->GetBytecode(bytecodeIndex + 1));
}

void
SOMppMethod::doPushGlobal(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	pushGlobal(builder, static_cast<VMSymbol*>(method->GetConstant(bytecodeIndex)));
}

void
SOMppMethod::doPop(TR::BytecodeBuilder *builder)
{
	POP(builder);
}

void
SOMppMethod::doPopLocal(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	uint8_t index = method->GetBytecode(bytecodeIndex + 1);
	uint8_t level = method->GetBytecode(bytecodeIndex + 2);
#if SOM_METHOD_DEBUG
	fprintf(stderr, " %d %d ", index, level);
#endif
	TR::IlValue *locals = nullptr;
	if (0 == level) {
		locals = builder->Load("frameLocals");
	} else {
		const char *contextName = getContext(builder, level);
		locals =
		builder->LoadIndirect("VMFrame", "locals",
		builder->	Load(contextName));
	}

	popValueToArray(builder, locals, index);
}

void
SOMppMethod::doPopArgument(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	/* see Interpreter::doPopArgument and VMFrame::SetArgument.
	 * level does not appear to be used. */
	popValueToArray(builder, builder->Load("frameArguments"), method->GetBytecode(bytecodeIndex + 1));
}

void
SOMppMethod::doPopField(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
#if SOM_METHOD_DEBUG
	fprintf(stderr, " %d ", method->GetBytecode(bytecodeIndex + 1));
#endif
	popField(builder, getSelf(builder), method->GetBytecode(bytecodeIndex + 1));
}

void
SOMppMethod::doSend(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex, TR::BytecodeBuilder *fallThrough)
{
	VMSymbol* signature = static_cast<VMSymbol*>(method->GetConstant(bytecodeIndex));
	int numOfArgs = Signature::GetNumberOfArguments(signature);
	TR::BytecodeBuilder *genericSend = nullptr;
	TR::BytecodeBuilder *merge = nullptr;

	INLINE_STATUS status = doInlineIfPossible(&builder, &genericSend, &merge, signature, bytecodeIndex);

	if (status != INLINE_FAILED) {
		builder->Goto(merge);
	} else {
		genericSend = builder;
		merge = builder;
	}

	if (status != INLINE_SUCCESSFUL_NO_GENERIC_PATH_REQUIRED) {
		genericSend->Store("receiverClass",
		genericSend->	Call("getClass", 1, PICK(genericSend, numOfArgs - 1)));

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

		TR::IlBuilder *bail = nullptr;
		genericSend->IfThen(&bail,
		genericSend->	EqualTo(
		genericSend->		Load("return"),
		genericSend->		ConstInt64(-1)));

		justReturn(bail);

		genericSend->Store("sendResult",
		genericSend->	LoadAt(pInt64,
		genericSend->		LoadIndirect("VMFrame", "stack_ptr",
		genericSend->			Load("frame"))));

		TR::BytecodeBuilder *restartIfRequired = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
		genericSend->IfCmpNotEqual(&restartIfRequired,
		genericSend->	Load("return"),
		genericSend->	ConstInt64((int64_t)bytecodeIndex));

		DROPALL(restartIfRequired);

		TR::BytecodeBuilder *start = bytecodeBuilderTable[0];
		restartIfRequired->Goto(start);

		if (status != INLINE_FAILED) {
			genericSend->Goto(merge);
		}
	}

	DROP(merge, numOfArgs);
	PUSH(merge, merge->Load("sendResult"));
	merge->AddFallThroughBuilder(fallThrough);
}

void
SOMppMethod::doSuperSend(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex)
{
	VMSymbol* signature = static_cast<VMSymbol*>(method->GetConstant(bytecodeIndex));
	int numOfArgs = Signature::GetNumberOfArguments(signature);

#if SOM_METHOD_DEBUG
	char *signatureChars = signature->GetChars();
	fprintf(stderr, " %s ", signatureChars);
#endif

	COMMIT(builder);

	builder->Store("return",
	builder->	Call("doSuperSendHelper", 4,
	builder->		Load("interpreter"),
	builder->		Load("frame"),
	builder->		ConstInt64((int64_t)signature),
	builder->		ConstInt64((int64_t)bytecodeIndex)));

	TR::IlBuilder *bail = nullptr;
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
}

void
SOMppMethod::doReturnNonLocal(TR::BytecodeBuilder *builder, long bytecodeIndex)
{
	TR::IlValue *result = POP(builder);

	builder->Store("return",
	builder->	Call("popToContext", 2,
	builder->		Load("interpreter"),
	builder->		Load("frame")));

	TR::IlBuilder *continuePath = nullptr;
	TR::IlBuilder *didEscapedSend = nullptr;

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
}

void
SOMppMethod::doJumpIfFalse(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex)
{
#if SOM_METHOD_DEBUG
	fprintf(stderr, " jump to %lu ", calculateBytecodeIndexForJump(method, bytecodeIndex));
#endif
	TR::IlValue *value = POP(builder);
	TR::BytecodeBuilder *destBuilder = bytecodeBuilderTable[calculateBytecodeIndexForJump(method, bytecodeIndex)];
	builder->IfCmpEqual(&destBuilder, value,
	builder->	ConstInt64((int64_t)falseObject));
}

void
SOMppMethod::doJumpIfTrue(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex)
{
#if SOM_METHOD_DEBUG
	fprintf(stderr, " jump to %lu ", calculateBytecodeIndexForJump(method, bytecodeIndex));
#endif
	TR::IlValue *value = POP(builder);
	TR::BytecodeBuilder *destBuilder = bytecodeBuilderTable[calculateBytecodeIndexForJump(method, bytecodeIndex)];
	builder->IfCmpEqual(&destBuilder, value,
	builder->	ConstInt64((int64_t)trueObject));
}

void
SOMppMethod::doJump(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex)
{
#if SOM_METHOD_DEBUG
	fprintf(stderr, " jump to %lu ", calculateBytecodeIndexForJump(method, bytecodeIndex));
#endif
	TR::BytecodeBuilder *destBuilder = bytecodeBuilderTable[calculateBytecodeIndexForJump(method, bytecodeIndex)];
	builder->Goto(destBuilder);
}

void
SOMppMethod::pushValueFromArray(TR::BytecodeBuilder *builder, TR::IlValue *array, uint8_t arrayIndex)
{
	TR::IlValue *value =
	builder->LoadAt(pInt64,
	builder->	IndexAt(pInt64,
	builder->		ConvertTo(pInt64, array),
	builder->		ConstInt64(arrayIndex)));

	PUSH(builder, value);
}

void
SOMppMethod::pushField(TR::BytecodeBuilder *builder, TR::IlValue *object, uint8_t fieldIndex)
{
	if (fieldIndex < FIELDNAMES_LENGTH) {
		const char *fieldName = fieldNames[fieldIndex];
		PUSH(builder, builder->LoadIndirect("VMObject", fieldName, object));
	} else {
		TR::IlValue *field =
		builder->Call("getFieldFrom", 2, object,
		builder->	ConstInt64((int64_t)fieldIndex));
		PUSH(builder, field);
	}
}

void
SOMppMethod::pushConstant(TR::BytecodeBuilder *builder, VMMethod *vmMethod, uint8_t constantIndex)
{
	PUSH(builder, builder->ConstInt64((int64_t)vmMethod->indexableFields[constantIndex]));
}

void
SOMppMethod::pushGlobal(TR::BytecodeBuilder *builder, VMSymbol* globalName)
{
	TR::IlValue *global =
	builder->	Call("getGlobal", 1,
	builder->		ConstInt64((int64_t)globalName));

	TR::IlBuilder *globalIsNullPtr = nullptr;
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
}

void
SOMppMethod::popValueToArray(TR::BytecodeBuilder *builder, TR::IlValue *array, uint8_t arrayIndex)
{
	TR::IlValue *value = POP(builder);
	builder->StoreAt(
	builder->	IndexAt(pInt64,
	builder->		ConvertTo(pInt64, array),
	builder->		ConstInt64(arrayIndex)), value);
}

void
SOMppMethod::popField(TR::BytecodeBuilder *builder, TR::IlValue *object, uint8_t fieldIndex)
{
	TR::IlValue *value = POP(builder);
	if (fieldIndex < FIELDNAMES_LENGTH) {
		const char *fieldName = fieldNames[fieldIndex];
		builder->StoreIndirect("VMObject", fieldName, object, value);
	} else {
		builder->Call("setFieldTo", 3, object,
		builder->	ConstInt64((int64_t)fieldIndex), value);
	}
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
			TR::IlBuilder *iloop = nullptr;
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
SOMppMethod::getSelf(TR::IlBuilder *builder)
{
	TR::IlValue *context = builder->Load("frameOuterContext");
	TR::IlValue *self =
	builder->LoadAt(pInt64,
	builder->	IndexAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			LoadIndirect("VMFrame", "arguments", context)),
	builder->		ConstInt64(0)));

	return self;
}

static const char *getArgumentName(int32_t recursiveLevel)
{
	if (recursiveLevel == 0) {
		return "argumentsArray";
	} else if (recursiveLevel == 1) {
		return "argumentsArray1";
	} else if (recursiveLevel == 2) {
		return "argumentsArray2";
	} else if (recursiveLevel == 3) {
		return "argumentsArray3";
	} else if (recursiveLevel == 4) {
		return "argumentsArray4";
	} else {
		return nullptr;
	}
}

static const char *getLocalName(int32_t recursiveLevel)
{
	if (recursiveLevel == 0) {
		return "localsArray";
	} else if (recursiveLevel == 1) {
		return "localsArray1";
	} else if (recursiveLevel == 2) {
		return "localsArray2";
	} else if (recursiveLevel == 3) {
		return "localsArray3";
	} else if (recursiveLevel == 4) {
		return "localsArray4";
	} else {
		return nullptr;
	}
}

SOMppMethod::INLINE_STATUS
SOMppMethod::doInlineIfPossible(TR::BytecodeBuilder **builder, TR::BytecodeBuilder **genericSend, TR::BytecodeBuilder **merge, VMSymbol *signature, long bytecodeIndex)
{
	VMClass *receiverFromCache = method->getInvokeReceiverCache(bytecodeIndex);
	const char *signatureChars = signature->GetChars();
	INLINE_STATUS status = INLINE_FAILED;

#if SOM_METHOD_DEBUG
	const char* receiverClassName = ((GCObject*)receiverFromCache == nullptr) ? "(unknown class)" : receiverFromCache->GetName()->GetChars();
#endif

	if (doInlining) {
		if (nullptr != receiverFromCache) {
			VMInvokable *invokable = receiverFromCache->LookupInvokable(signature);
			if (nullptr != invokable) {
				VMClass *invokableClass = invokable->GetHolder();
				SOMppMethod::RECOGNIZED_METHOD_INDEX recognizedMethodIndex = getRecognizedMethodIndex(method, receiverFromCache, invokableClass, signatureChars, 0);
				stackTopForErrorHandling = GET_STACKTOP(*builder);
				if (NOT_RECOGNIZED != recognizedMethodIndex) {
					createBuildersForInlineSends(genericSend, merge, bytecodeIndex);
					status = generateRecognizedMethod(*builder, genericSend, recognizedMethodIndex, receiverFromCache, bytecodeIndex, 0);
				} else {
					if (!invokable->IsPrimitive()) {
						VMMethod *methodToInline = static_cast<VMMethod*>(invokable);
						if (methodIsInlineable(methodToInline, 0)) {
							createBuildersForInlineSends(genericSend, merge, bytecodeIndex);
							generateGenericMethod(builder, genericSend, invokable, receiverFromCache, signature, bytecodeIndex, 0);
							status = INLINE_SUCCESSFUL;
						}
					}
				}
			}
		}
	}

#if SOM_METHOD_DEBUG
	fprintf(stderr, " %s>>#%s %s ", receiverClassName, signatureChars, (status != INLINE_FAILED) ? "inlined" : "");
#endif

	return status;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateRecognizedMethod(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, SOMppMethod::RECOGNIZED_METHOD_INDEX recognizedMethodIndex, VMClass *receiverFromCache, long bytecodeIndex, int32_t recursiveLevel)
{
	INLINE_STATUS status = INLINE_STATUS::INLINE_FAILED;

	switch(recognizedMethodIndex) {
	case OBJECT_EQUAL:
		status = generateInlineForObjectEqual(builder, genericSend, receiverFromCache);
		break;
	case OBJECT_NOTEQUAL:
		status = generateInlineForObjectNotEqual(builder, genericSend, receiverFromCache);
		break;
	case OBJECT_VALUE:
		status = generateInlineForObjectValue(builder, genericSend, receiverFromCache);
		break;
	case GENERIC_ISNIL:
		status = generateInlineForGenericIsNil(builder);
		break;
	case GENERIC_NOTNIL:
		status = generateInlineForGenericNotNil(builder);
		break;
	case INTEGER_PLUS:
		status = generateInlineForIntegerMath(builder, genericSend, &SOMppMethod::add);
		break;
	case INTEGER_MINUS:
		status = generateInlineForIntegerMath(builder, genericSend, &SOMppMethod::sub);
		break;
	case INTEGER_MULTIPLY:
		status = generateInlineForIntegerMath(builder, genericSend, &SOMppMethod::mul);
		break;
	case INTEGER_DIVIDE:
		status = generateInlineForIntegerMath(builder, genericSend, &SOMppMethod::div);
		break;
	case INTEGER_PERCENT:
		status = generateInlineForIntegerMath(builder, genericSend, &SOMppMethod::percent);
		break;
	case INTEGER_AND:
		status = generateInlineForIntegerMath(builder, genericSend, &SOMppMethod::andVals);
		break;
	case INTEGER_LESSTHAN:
		status = generateInlineForIntegerBoolean(builder, genericSend, &SOMppMethod::lessThan, builder->ConstInt64((int64_t)trueObject), builder->ConstInt64((int64_t)falseObject));
		break;
	case INTEGER_LESSTHANEQUAL:
		status = generateInlineForIntegerBoolean(builder, genericSend, &SOMppMethod::greaterThan, builder->ConstInt64((int64_t)falseObject), builder->ConstInt64((int64_t)trueObject));
		break;
	case INTEGER_GREATERTHAN:
		status = generateInlineForIntegerBoolean(builder, genericSend, &SOMppMethod::greaterThan, builder->ConstInt64((int64_t)trueObject), builder->ConstInt64((int64_t)falseObject));
		break;
	case INTEGER_GREATERTHANEQUAL:
		status = generateInlineForIntegerBoolean(builder, genericSend, &SOMppMethod::lessThan, builder->ConstInt64((int64_t)falseObject), builder->ConstInt64((int64_t)trueObject));
		break;
	case INTEGER_EQUAL:
		status = generateInlineForIntegerBoolean(builder, genericSend, &SOMppMethod::equalTo, builder->ConstInt64((int64_t)trueObject), builder->ConstInt64((int64_t)falseObject));
		break;
	case INTEGER_NOTEQUAL:
		status = generateInlineForIntegerBoolean(builder, genericSend, &SOMppMethod::equalTo, builder->ConstInt64((int64_t)falseObject), builder->ConstInt64((int64_t)trueObject));
		break;
	case INTEGER_NEGATED:
		status = generateInlineForIntegerNegated(builder, genericSend);
		break;
	case INTEGER_MAX:
		status = generateInlineForIntegerMax(builder, genericSend);
		break;
	case INTEGER_ABS:
		status = generateInlineForIntegerAbs(builder, genericSend, bytecodeIndex);
		break;
	case INTEGER_TODO:
		status = generateInlineForIntegerToDo(builder, genericSend, bytecodeIndex, recursiveLevel);
		break;
	case INTEGER_TOBYDO:
		status = generateInlineForIntegerToByDo(builder, genericSend, bytecodeIndex, recursiveLevel);
		break;
	case INTEGER_DOWNTODO:
		status = generateInlineForIntegerDownToDo(builder, genericSend, bytecodeIndex, recursiveLevel);
		break;
	case INTEGER_DOWNTOBYDO:
		status = generateInlineForIntegerDownToByDo(builder, genericSend, bytecodeIndex, recursiveLevel);
		break;
	case ARRAY_AT:
		status = generateInlineForArrayAt(builder, genericSend);
		break;
	case ARRAY_ATPUT:
		status = generateInlineForArrayAtPut(builder, genericSend);
		break;
	case ARRAY_LENGTH:
		status = generateInlineForArrayLength(builder, genericSend);
		break;
	case DOUBLE_PLUS:
		status = generateInlineForDoubleMath(builder, genericSend, &SOMppMethod::add);
		break;
	case DOUBLE_MINUS:
		status = generateInlineForDoubleMath(builder, genericSend, &SOMppMethod::sub);
		break;
	case DOUBLE_MULTIPLY:
		status = generateInlineForDoubleMath(builder, genericSend, &SOMppMethod::mul);
		break;
	case DOUBLE_DIVIDE:
		status = generateInlineForDoubleMath(builder, genericSend, &SOMppMethod::div);
		break;
	case DOUBLE_LESSTHAN:
		status = generateInlineForDoubleBoolean(builder, genericSend, &SOMppMethod::lessThan, builder->ConstInt64((int64_t)trueObject), builder->ConstInt64((int64_t)falseObject));
		break;
	case DOUBLE_LESSTHANEQUAL:
		status = generateInlineForDoubleBoolean(builder, genericSend, &SOMppMethod::greaterThan, builder->ConstInt64((int64_t)falseObject), builder->ConstInt64((int64_t)trueObject));
		break;
	case DOUBLE_GREATERTHAN:
		status = generateInlineForDoubleBoolean(builder, genericSend, &SOMppMethod::greaterThan, builder->ConstInt64((int64_t)trueObject), builder->ConstInt64((int64_t)falseObject));
		break;
	case DOUBLE_GREATERTHANEQUAL:
		status = generateInlineForDoubleBoolean(builder, genericSend, &SOMppMethod::lessThan, builder->ConstInt64((int64_t)falseObject), builder->ConstInt64((int64_t)trueObject));
		break;
	case BLOCK_WHILETRUE:
		status = generateInlineForWhileTrue(builder, genericSend, bytecodeIndex, recursiveLevel);
		break;
	case BLOCK_WHILEFALSE:
		status = generateInlineForWhileFalse(builder, genericSend, bytecodeIndex, recursiveLevel);
		break;
	case BOOLEAN_AND:
		status = generateInlineForBooleanAnd(builder, genericSend, bytecodeIndex);
		break;
	case BOOLEAN_OR:
		status = generateInlineForBooleanOr(builder, genericSend, bytecodeIndex);
		break;
	case OSR_TO_GENERIC_SEND:
		status = generateInlineOSRToGenericSend(builder, genericSend, bytecodeIndex);
		break;
	case NOT_RECOGNIZED:
	default:
		/* TODO CHANGE TO RUNTIME ASSERT */
		fprintf(stderr, "Error generating recognized method %d\n",recognizedMethodIndex);
		int *x = 0;
		*x = 0;
	}

	return status;
}

/* Only used for INLINING */
TR::IlValue *
SOMppMethod::getLocalArrayForLevel(TR::BytecodeBuilder *builder, VMMethod *vmMethod, long bytecodeIndex, int32_t recursiveLevel)
{
	uint8_t level = vmMethod->GetBytecode(bytecodeIndex + 2);
	if (level == 0) {
		return builder->Load(getLocalName(recursiveLevel));
	} else if (level == 1) {
		if (recursiveLevel > 0) {
			return builder->Load(getLocalName(recursiveLevel - 1));
		} else {
			return builder->Load("frameLocals");
		}
	} else if (level == 2) {
		if (recursiveLevel > 1) {
			return builder->Load(getLocalName(recursiveLevel - 2));
		} else if (recursiveLevel == 1) {
			return builder->Load("frameArguments");
		} else {
			return builder->Load("frameContextLocals");
		}
	}
	return nullptr;
}

/* Only used for INLINING */
TR::IlValue *
SOMppMethod::getArgumentArrayForLevel(TR::BytecodeBuilder *builder, VMMethod *vmMethod, long bytecodeIndex, int32_t recursiveLevel)
{
	uint8_t level = vmMethod->GetBytecode(bytecodeIndex + 2);
	if (level == 0) {
		return builder->Load(getArgumentName(recursiveLevel));
	} else if (level == 1) {
		if (recursiveLevel > 0) {
			return builder->Load(getArgumentName(recursiveLevel - 1));
		} else {
			return builder->Load("frameArguments");
		}
	} else if (level == 2) {
		if (recursiveLevel > 1) {
			return builder->Load(getArgumentName(recursiveLevel - 2));
		} else if (recursiveLevel == 1) {
			return builder->Load("frameArguments");
		} else {
			return builder->Load("frameContextArguments");
		}
	}
	return nullptr;
}

void
SOMppMethod::generateGenericMethodBody(TR::BytecodeBuilder **ibuilder, TR::BytecodeBuilder **genericSend, VMMethod *methodToInline, TR::IlValue *receiver, long bytecodeIndex, int32_t recursiveLevel)
{
	long bytecodeCount = methodToInline->GetNumberOfBytecodes();
	long bcIndex = 0;
	long tableSize = sizeof(TR::BytecodeBuilder *) * bytecodeCount;
	TR::BytecodeBuilder **bytecodeBuilderTable = (TR::BytecodeBuilder **)malloc(tableSize);
	if (nullptr != bytecodeBuilderTable) {
		memset(bytecodeBuilderTable, 0, tableSize);
		long i = 0;
		while (i < bytecodeCount) {
			uint8_t bc = methodToInline->GetBytecode(i);
			bytecodeBuilderTable[i] = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
			i += Bytecode::GetBytecodeLength(bc);
		}
	}
	int* bytecodeTableEntryHasBeenReached = (int*)malloc(sizeof(int) * bytecodeCount);
	if (nullptr != bytecodeTableEntryHasBeenReached) {
		memset(bytecodeTableEntryHasBeenReached, 0, sizeof(int)*bytecodeCount);
	}

	TR::BytecodeBuilder *merge = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	TR::BytecodeBuilder *previousBuilder = *ibuilder;

	while (bcIndex < bytecodeCount) {
		TR::BytecodeBuilder *builder = bytecodeBuilderTable[bcIndex];
		if (NULL != previousBuilder) {
			previousBuilder->Goto(builder);
			bytecodeTableEntryHasBeenReached[bcIndex] = 1;
		}
		uint8_t bc = methodToInline->GetBytecode(bcIndex);
		if (0 == bytecodeTableEntryHasBeenReached[bcIndex]) {
			//handle unreachable code
			bcIndex += Bytecode::GetBytecodeLength(bc);
			continue;
		}
		switch(bc) {
		case BC_DUP:
			DUP(builder);
			break;
		case BC_PUSH_LOCAL:
			pushValueFromArray(builder, getLocalArrayForLevel(builder, methodToInline, bcIndex, recursiveLevel), methodToInline->GetBytecode(bcIndex + 1));
			break;
		case BC_PUSH_FIELD:
			pushField(builder, receiver, methodToInline->GetBytecode(bcIndex + 1));
			break;
		case BC_PUSH_CONSTANT:
			pushConstant(builder, methodToInline, methodToInline->GetBytecode(bcIndex + 1));
			break;
		case BC_PUSH_GLOBAL:
			pushGlobal(builder, static_cast<VMSymbol*>(methodToInline->GetConstant(bcIndex)));
			break;
		case BC_PUSH_ARGUMENT:
			pushValueFromArray(builder, getArgumentArrayForLevel(builder, methodToInline, bcIndex, recursiveLevel), methodToInline->GetBytecode(bcIndex + 1));
			break;
		case BC_POP:
			POP(builder);
			break;
		case BC_POP_LOCAL:
			popValueToArray(builder, getLocalArrayForLevel(builder, methodToInline, bcIndex, recursiveLevel), methodToInline->GetBytecode(bcIndex + 1));
			break;
		case BC_POP_FIELD:
			popField(builder, receiver, methodToInline->GetBytecode(bcIndex + 1));
			break;
		case BC_POP_ARGUMENT:
			popValueToArray(builder, getArgumentArrayForLevel(builder, methodToInline, bcIndex, recursiveLevel), methodToInline->GetBytecode(bcIndex + 1));
			break;
		case BC_SEND:
		{
			VMSymbol* signature = static_cast<VMSymbol*>(methodToInline->GetConstant(bcIndex));
			int32_t numOfArgs = Signature::GetNumberOfArguments(signature);
			VMClass *receiverFromCache = methodToInline->getInvokeReceiverCache(bcIndex);
			VMClass *invokableClass = NULL;
			if (NULL != receiverFromCache) {
				invokableClass = receiverFromCache->LookupInvokable(signature)->GetHolder();
			}
			SOMppMethod::RECOGNIZED_METHOD_INDEX recognizedMethodIndex = getRecognizedMethodIndex(methodToInline, receiverFromCache, invokableClass, signature->GetChars(), recursiveLevel + 1);
			if (NOT_RECOGNIZED != recognizedMethodIndex) {
				generateRecognizedMethod(builder, genericSend, recognizedMethodIndex, receiverFromCache, bcIndex, recursiveLevel + 1);
			} else {
				generateGenericMethod(&builder, genericSend, receiverFromCache->LookupInvokable(signature), receiverFromCache, signature, bytecodeIndex, recursiveLevel + 1);
			}
			DROP(builder, numOfArgs);
			PUSH(builder, builder->Load("sendResult"));
			break;
		}
		case BC_JUMP_IF_FALSE:
		{
			TR::IlValue *value = POP(builder);
			TR::BytecodeBuilder *destBuilder = bytecodeBuilderTable[calculateBytecodeIndexForJump(methodToInline, bcIndex)];
			builder->IfCmpEqual(&destBuilder, value,
			builder->	ConstInt64((int64_t)falseObject));
			bytecodeTableEntryHasBeenReached[calculateBytecodeIndexForJump(methodToInline, bcIndex)] = 1;
			break;
		}
		case BC_JUMP_IF_TRUE:
		{
			TR::IlValue *value = POP(builder);
			TR::BytecodeBuilder *destBuilder = bytecodeBuilderTable[calculateBytecodeIndexForJump(methodToInline, bcIndex)];
			builder->IfCmpEqual(&destBuilder, value,
			builder->	ConstInt64((int64_t)trueObject));
			bytecodeTableEntryHasBeenReached[calculateBytecodeIndexForJump(methodToInline, bcIndex)] = 1;
			break;
		}
		case BC_JUMP:
		{
			TR::BytecodeBuilder *destBuilder = bytecodeBuilderTable[calculateBytecodeIndexForJump(methodToInline, bcIndex)];
			builder->Goto(&destBuilder);
			builder = NULL;
			bytecodeTableEntryHasBeenReached[calculateBytecodeIndexForJump(methodToInline, bcIndex)] = 1;
			break;
		}
		case BC_RETURN_LOCAL:
		{
			TR::IlValue *returnValue = POP(builder);
			builder->Store("sendResult", returnValue);
			builder->Goto(merge);
			builder = NULL;
			break;
		}
		case BC_RETURN_NON_LOCAL:
		{
			/* Return non local can only be inlined at recursive level 0 */
			TR::IlValue *result = POP(builder);

			builder->Call("popFrameAndPushResult", 3,
			builder->	Load("interpreter"),
			builder->	Load("frame"), result);

			justReturn(builder);
			builder = NULL;
			break;
		}
		default:
		{
			fprintf(stderr, "bad opcode in inliner\n");
			int *x = 0;
			*x = 0;
		}
		}
		bcIndex += Bytecode::GetBytecodeLength(bc);
		previousBuilder = builder;
	}
	*ibuilder = merge;
	free(bytecodeTableEntryHasBeenReached);
	free(bytecodeBuilderTable);
}

void
SOMppMethod::generateGenericMethod(TR::BytecodeBuilder **b, TR::BytecodeBuilder **genericSend, VMInvokable *invokable, VMClass *receiverClass, VMSymbol *signature, long bytecodeIndex, int32_t recursiveLevel)
{
	VMMethod *methodToInline = static_cast<VMMethod*>(invokable);
	long numOfArgs = methodToInline->GetNumberOfArguments();
	long numOfLocals = methodToInline->GetNumberOfLocals();
	TR::BytecodeBuilder *builder = *b;
	TR::IlValue *receiver = PICK(builder, numOfArgs - 1);
	const char *argumentName = getArgumentName(recursiveLevel);

	builder->Store(argumentName,
	builder->	CreateLocalArray((int32_t)numOfArgs, Int64));

	for (int32_t i = 0; i < (int32_t)numOfArgs; i++) {
		builder->StoreAt(
		builder->	IndexAt(pInt64,
		builder->		ConvertTo(pInt64,
		builder->			Load(argumentName)),
		builder->		ConstInt32(i)),
					PICK(builder, numOfArgs - 1 - i));
	}

	if (numOfLocals > 0) {
		builder->Store(getLocalName(recursiveLevel),
		builder->	CreateLocalArray((int32_t)numOfLocals, Int64));
	}

	verifyArg(builder, genericSend, receiver, builder->ConstInt64((int64_t)receiverClass));

	generateGenericMethodBody(b, genericSend, methodToInline, receiver, bytecodeIndex, recursiveLevel);
}

void
SOMppMethod::createBuildersForInlineSends(TR::BytecodeBuilder **genericSend, TR::BytecodeBuilder **merge, long bytecodeIndex)
{
	if (nullptr == *genericSend) {
		*genericSend = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	}
	if (nullptr == *merge) {
		*merge = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	}
}

void
SOMppMethod::verifyIntegerArg(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, TR::IlValue *integer)
{
#if USE_TAGGING
	TR::BytecodeBuilder *failurePath = nullptr;
	builder->IfCmpEqualZero(&failurePath,
	builder->	And(integer,
	builder->		ConstInt64(0x1)));

	SET_STACKTOP(failurePath, stackTopForErrorHandling);
	failurePath->Goto(*genericSend);
#else
	verifyArg(builder, genericSend, integer, builder->ConstInt64((int64_t)integerClass));
#endif
}

void
SOMppMethod::verifyDoubleArg(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, TR::IlValue *object, TR::IlValue *objectClass)
{
	builder->Store("isOk", builder->ConstInt32(1));
	TR::IlBuilder *notDouble = nullptr;
	builder->IfThen(&notDouble,
	builder->	NotEqualTo(objectClass, builder->ConstInt64((int64_t)doubleClass)));

	/*Inline verifyInteger */
#if USE_TAGGING
	TR::IlBuilder *notInteger = nullptr;
	notDouble->IfThen(&notInteger,
	notDouble->	NotEqualTo(
	notDouble->		And(object,
	notDouble->			ConstInt64(0x1)),
	notDouble->		ConstInt64(0x1)));

	notInteger->Store("isOk", notInteger->ConstInt32(0));
#else
	TR::IlBuilder *notInteger = nullptr;
	notDouble->IfThen(&notInteger,
	notDouble->	NotEqualTo(objectClass, notDouble->ConstInt64((int64_t)integerClass)));

	notInteger->Store("isOk", notInteger->ConstInt32(0));
#endif

	TR::BytecodeBuilder *failurePath = nullptr;
	builder->IfCmpNotEqual(&failurePath, builder->Load("isOk"), builder->ConstInt32(1));

	SET_STACKTOP(failurePath, stackTopForErrorHandling);
	failurePath->Goto(*genericSend);
}

void
SOMppMethod::verifyArg(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, TR::IlValue *object, TR::IlValue *type)
{
	TR::IlValue *objectClass =
	builder->Call("getClass", 1, object);

	TR::BytecodeBuilder *failurePath = nullptr;
	builder->IfCmpNotEqual(&failurePath, objectClass, type);

	SET_STACKTOP(failurePath, stackTopForErrorHandling);
	failurePath->Goto(*genericSend);
}

void
SOMppMethod::verifyBooleanArg(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, TR::IlValue *object)
{
	TR::IlValue *superClass =
	builder->Call("getSuperClass", 1, object);

	TR::BytecodeBuilder *failurePath = nullptr;
	builder->IfCmpNotEqual(&failurePath, superClass, builder->ConstInt64((int64_t)booleanClass));

	SET_STACKTOP(failurePath, stackTopForErrorHandling);
	failurePath->Goto(*genericSend);
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForIntegerMath(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, MathFuncType mathFunction)
{
	TR::IlValue *receiver = PICK(builder, 1);
	TR::IlValue *param1 = TOP(builder);

	verifyIntegerArg(builder, genericSend, receiver);
	verifyIntegerArg(builder, genericSend, param1);

	TR::IlValue *receiverValue = getIntegerValue(builder, receiver);
	TR::IlValue *param1Value = getIntegerValue(builder, param1);
	TR::IlValue *newValue = (*mathFunction)(builder, receiverValue, param1Value);
	TR::IlValue *integerObject = newIntegerObjectForValue(builder, genericSend, newValue);

	builder->Store("sendResult", integerObject);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForIntegerBoolean(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, BooleanFuncType booleanFunction, TR::IlValue *thenPathValue, TR::IlValue *elsePathValue)
{
	TR::IlValue *receiver = PICK(builder, 1);
	TR::IlValue *param1 = TOP(builder);

	verifyIntegerArg(builder, genericSend, receiver);
	verifyIntegerArg(builder, genericSend, param1);

	TR::IlValue *receiverValue = getIntegerValue(builder, receiver);
	TR::IlValue *param1Value = getIntegerValue(builder, param1);

	TR::IlBuilder *thenPath = nullptr;
	TR::IlBuilder *elsePath = nullptr;
	builder->IfThenElse(&thenPath, &elsePath, (*booleanFunction)(builder, receiverValue, param1Value));

	thenPath->Store("sendResult", thenPathValue);

	elsePath->Store("sendResult", elsePathValue);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForIntegerNegated(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend)
{
	TR::IlValue *receiver = TOP(builder);

	verifyIntegerArg(builder, genericSend, receiver);

	TR::IlValue *receiverValue = getIntegerValue(builder, receiver);

	TR::IlValue *newValue =
	builder->Sub(
	builder->	ConstInt64(0), receiverValue);

	TR::IlValue *integerObject = newIntegerObjectForValue(builder, genericSend, newValue);

	builder->Store("sendResult", integerObject);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForIntegerMax(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend)
{
	TR::IlValue *receiver = PICK(builder, 1);
	TR::IlValue *param1 = TOP(builder);

	verifyIntegerArg(builder, genericSend, receiver);
	verifyIntegerArg(builder, genericSend, param1);

	TR::IlValue *receiverValue = getIntegerValue(builder, receiver);
	TR::IlValue *param1Value = getIntegerValue(builder, param1);

	TR::IlBuilder *thenPath = nullptr;
	TR::IlBuilder *elsePath = nullptr;
	builder->IfThenElse(&thenPath, &elsePath,
	builder->	GreaterThan(receiverValue, param1Value));

	thenPath->Store("sendResult", receiver);

	elsePath->Store("sendResult", param1);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForIntegerAbs(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, long bytecodeIndex)
{
	TR::IlValue *receiver = TOP(builder);

	verifyIntegerArg(builder, genericSend, receiver);

	builder->Store("absValue", getIntegerValue(builder, receiver));

	TR::IlBuilder *thenPath = nullptr;
	builder->IfThen(&thenPath,
	builder->	LessThan(
	builder->		Load("absValue"),
	builder->		ConstInt64(0)));

	thenPath->Store("absValue",
	thenPath->	Sub(
	thenPath->		ConstInt64(0),
	thenPath->		Load("absValue")));

	builder->Store("sendResult", newIntegerObjectForValue(builder, genericSend, builder->Load("absValue")));

	return INLINE_SUCCESSFUL;
}

void
SOMppMethod::generateInlineForIntegerLoop(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, long bytecodeIndex, ForLoopFuncType loopFunction, TR::IlValue *start, TR::IlValue *end, TR::IlValue *increment, TR::IlValue *block, VMMethod *blockToInline, int32_t recursiveLevel)
{
	const char *arguements = getArgumentName(recursiveLevel);

	builder->Store(arguements,
	builder->	CreateLocalArray((int32_t)2, Int64));

	long numOfLocals = blockToInline->GetNumberOfLocals();
	if (numOfLocals > 0) {
		builder->Store(getLocalName(recursiveLevel),
		builder->	CreateLocalArray((int32_t)numOfLocals, Int64));
	}

	builder->StoreAt(
	builder->	IndexAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			Load(arguements)),
	builder->		ConstInt32(0)), block);

	TR::BytecodeBuilder *iloop = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	builder->AddSuccessorBuilder(&iloop);

	TR::BytecodeBuilder *first = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	TR::BytecodeBuilder *last = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));

	iloop->AddSuccessorBuilder(&first);
	iloop->AppendBuilder(first);
	iloop->AddSuccessorBuilder(&last);
	iloop->AppendBuilder(last);

	// Initialize the loop after the loop code has been created!
	TR::IlBuilder *looper = (TR::IlBuilder *)iloop;
	loopFunction(builder, "i", &looper, start, end, increment);

	/* START OF LOOP */
	TR::IlValue *i = first->Load("i");
	TR::IlValue *iValue = newIntegerObjectForValue(first, genericSend, i);

	first->StoreAt(
	first-> IndexAt(pInt64,
	first->		ConvertTo(pInt64,
	first->         Load(arguements)),
	first->         ConstInt32(1)), iValue);

#if SOM_METHOD_DEBUG
	fprintf(stderr, " %s>>#%s ", blockToInline->GetHolder()->GetName()->GetChars(), blockToInline->GetSignature()->GetChars());
#endif

	generateGenericMethodBody(&first, genericSend, blockToInline, getSelf(iloop), bytecodeIndex, recursiveLevel);

	first->Goto(last);
	/* END OF LOOP */

	TR::IlValue *locals = builder->Load("frameLocals");
	TR::IlValue *value =
	builder->LoadAt(pInt64,
	builder->	IndexAt(pInt64,
	builder->		ConvertTo(pInt64, locals),
	builder->		ConstInt64(0)));

	builder->Store("sendResult", value);
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForIntegerToByDo(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, long bytecodeIndex, int32_t recursiveLevel)
{
	TR::IlValue *loopStart = PICK(builder, 3);
	TR::IlValue *loopEnd = PICK(builder, 2);
	TR::IlValue *loopIncrement = PICK(builder, 1);
	TR::IlValue *block = PICK(builder, 0);

	verifyIntegerArg(builder, genericSend, loopStart);
	verifyIntegerArg(builder, genericSend, loopEnd);
	verifyIntegerArg(builder, genericSend, loopIncrement);

	TR::IlValue *loopStartValue = getIntegerValue(builder, loopStart);
	TR::IlValue *loopEndValue = getIntegerValue(builder, loopEnd);
	/* need to add 1 to the loop end */
	TR::IlValue *actualLoopEnd = builder->Add(loopEndValue, builder->ConstInt64(1));
	TR::IlValue *loopIncrementValue = getIntegerValue(builder, loopIncrement);

	generateInlineForIntegerLoop(builder, genericSend, bytecodeIndex, &SOMppMethod::forLoopUp, loopStartValue, actualLoopEnd, loopIncrementValue, block, forLoopBlock, recursiveLevel);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForIntegerToDo(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, long bytecodeIndex, int32_t recursiveLevel)
{
	TR::IlValue *loopStart = PICK(builder, 2); /* receiver for the Integer>>#to:do: (initial loop) */
	TR::IlValue *loopEnd = PICK(builder, 1); /* the integer for to: (loop end) */
	TR::IlValue *block = PICK(builder, 0); /* the block */

	verifyIntegerArg(builder, genericSend, loopStart);
	verifyIntegerArg(builder, genericSend, loopEnd);

	TR::IlValue *loopStartValue = getIntegerValue(builder, loopStart);
	TR::IlValue *loopEndValue = getIntegerValue(builder, loopEnd);
	/* need to add 1 to the loop end */
	TR::IlValue *actualLoopEnd = builder->Add(loopEndValue, builder->ConstInt64(1));
	TR::IlValue *loopIncrementValue = builder->ConstInt64(1);

	generateInlineForIntegerLoop(builder, genericSend, bytecodeIndex, &SOMppMethod::forLoopUp, loopStartValue, actualLoopEnd, loopIncrementValue, block, forLoopBlock, recursiveLevel);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForIntegerDownToDo(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, long bytecodeIndex, int32_t recursiveLevel)
{
	TR::IlValue *loopStart = PICK(builder, 2); /* receiver for the Integer>>#to:do: (initial loop) */
	TR::IlValue *loopEnd = PICK(builder, 1); /* the integer for to: (loop end) */
	TR::IlValue *block = PICK(builder, 0); /* the block */

	verifyIntegerArg(builder, genericSend, loopStart);
	verifyIntegerArg(builder, genericSend, loopEnd);

	TR::IlValue *loopStartValue = getIntegerValue(builder, loopStart);
	TR::IlValue *loopEndValue = getIntegerValue(builder, loopEnd);
	TR::IlValue *loopIncrementValue = builder->ConstInt64(1);

	generateInlineForIntegerLoop(builder, genericSend, bytecodeIndex, &SOMppMethod::forLoopDown, loopStartValue, loopEndValue, loopIncrementValue, block, forLoopBlock, recursiveLevel);

	return INLINE_SUCCESSFUL;
}
SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForIntegerDownToByDo(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, long bytecodeIndex, int32_t recursiveLevel)
{
	TR::IlValue *loopStart = PICK(builder, 3);
	TR::IlValue *loopEnd = PICK(builder, 2);
	TR::IlValue *loopIncrement = PICK(builder, 1);
	TR::IlValue *block = PICK(builder, 0);

	verifyIntegerArg(builder, genericSend, loopStart);
	verifyIntegerArg(builder, genericSend, loopEnd);
	verifyIntegerArg(builder, genericSend, loopIncrement);

	TR::IlValue *loopStartValue = getIntegerValue(builder, loopStart);
	TR::IlValue *loopEndValue = getIntegerValue(builder, loopEnd);
	TR::IlValue *loopIncrementValue = getIntegerValue(builder, loopIncrement);

	generateInlineForIntegerLoop(builder, genericSend, bytecodeIndex, &SOMppMethod::forLoopDown, loopStartValue, loopEndValue, loopIncrementValue, block, forLoopBlock, recursiveLevel);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForArrayAt(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend)
{
	TR::IlValue *array = PICK(builder, 1);
	TR::IlValue *index = PICK(builder, 0);

	verifyArg(builder, genericSend, array, builder->ConstInt64((int64_t)arrayClass));
	verifyIntegerArg(builder, genericSend, index);

	builder->Store("sendResult",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64, getIndexableFieldSlot(builder, array, index))));

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForArrayAtPut(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend)
{
	TR::IlValue *array = PICK(builder, 2);
	TR::IlValue *index = PICK(builder, 1);
	TR::IlValue *object = PICK(builder, 0);

	verifyArg(builder, genericSend, array, builder->ConstInt64((int64_t)arrayClass));
	verifyIntegerArg(builder, genericSend, index);

	builder->StoreAt(
	builder->	ConvertTo(pInt64, getIndexableFieldSlot(builder, array, index)), object);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForArrayLength(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend)
{
	TR::IlValue *array = PICK(builder, 0);

	verifyArg(builder, genericSend, array, builder->ConstInt64((int64_t)arrayClass));

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

	TR::IlValue *integerObject = newIntegerObjectForValue(builder, genericSend, numberOfIndexableFields);

	builder->Store("sendResult", integerObject);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForDoubleMath(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, MathFuncType mathFunction)
{
	TR::IlValue *receiver = PICK(builder, 1);
	TR::IlValue *param1 = TOP(builder);

	verifyArg(builder, genericSend, receiver, builder->ConstInt64((int64_t)doubleClass));

	TR::IlValue *param1Class = builder->Call("getClass", 1, param1);
	verifyDoubleArg(builder, genericSend, param1, param1Class);
//	verifyArg(builder, genericSend, param1, builder->ConstInt64((int64_t)doubleClass));

	TR::IlValue *receiverValue = getDoubleValue(builder, receiver);
	TR::IlValue *param1Value = getDoubleValueFromDoubleOrInteger(builder, param1, param1Class);
//	TR::IlValue *param1Value = getDoubleValue(builder, param1);
	TR::IlValue *newValue = (*mathFunction)(builder, receiverValue, param1Value);

	builder->Store("sendResult",
	builder->	Call("newDouble", 1, newValue));

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForDoubleBoolean(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, BooleanFuncType booleanFunction, TR::IlValue *thenPathValue, TR::IlValue *elsePathValue)
{
	TR::IlValue *receiver = PICK(builder, 1);
	TR::IlValue *param1 = TOP(builder);

	verifyArg(builder, genericSend, receiver, builder->ConstInt64((int64_t)doubleClass));

//	TR::IlValue *param1Class = builder->Call("getClass", 1, param1);
//	verifyDoubleArg(builder, genericSend, param1, param1Class);
	verifyArg(builder, genericSend, param1, builder->ConstInt64((int64_t)doubleClass));

	TR::IlValue *receiverValue = getDoubleValue(builder, receiver);
//	TR::IlValue *param1Value = getDoubleValueFromDoubleOrInteger(builder, param1, param1Class);
	TR::IlValue *param1Value = getDoubleValue(builder, param1);

	TR::IlBuilder *thenPath = nullptr;
	TR::IlBuilder *elsePath = nullptr;
	builder->IfThenElse(&thenPath, &elsePath, (*booleanFunction)(builder, receiverValue, param1Value));

	thenPath->Store("sendResult", thenPathValue);

	elsePath->Store("sendResult", elsePathValue);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForObjectNotEqual(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, VMClass *receiverClass)
{
	TR::IlValue *receiver = PICK(builder, 1);
	TR::IlValue *param1 = PICK(builder, 0);

	verifyArg(builder, genericSend, receiver, builder->ConstInt64((int64_t)receiverClass));

	TR::IlBuilder *thenPath = nullptr;
	TR::IlBuilder *elsePath = nullptr;
	builder->IfThenElse(&thenPath, &elsePath,
	builder->	NotEqualTo(receiver, param1));

	thenPath->Store("sendResult",
	thenPath->	ConstInt64((int64_t)trueObject));

	elsePath->Store("sendResult",
	elsePath->	ConstInt64((int64_t)falseObject));

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForObjectEqual(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, VMClass *receiverClass)
{
	TR::IlValue *receiver = PICK(builder, 1);
	TR::IlValue *param1 = PICK(builder, 0);

	verifyArg(builder, genericSend, receiver, builder->ConstInt64((int64_t)receiverClass));

	TR::IlBuilder *thenPath = nullptr;
	TR::IlBuilder *elsePath = nullptr;
	builder->IfThenElse(&thenPath, &elsePath,
	builder->	EqualTo(receiver, param1));

	thenPath->Store("sendResult",
	thenPath->	ConstInt64((int64_t)trueObject));

	elsePath->Store("sendResult",
	elsePath->	ConstInt64((int64_t)falseObject));

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForObjectValue(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, VMClass *receiverClass)
{
	TR::IlValue *receiver = PICK(builder, 0);

	verifyArg(builder, genericSend, receiver, builder->ConstInt64((int64_t)receiverClass));

	builder->Store("sendResult", receiver);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForGenericIsNil(TR::BytecodeBuilder *builder)
{
	TR::IlValue *receiver = PICK(builder, 0);

	TR::IlBuilder *thenPath = nullptr;
	TR::IlBuilder *elsePath = nullptr;
	builder->IfThenElse(&thenPath, &elsePath,
	builder->	EqualTo(receiver,
	builder->		ConstInt64((int64_t)nilObject)));

	/* store true result */
	thenPath->Store("sendResult",
	thenPath->	ConstInt64((int64_t)trueObject));

	/* store false result */
	elsePath->Store("sendResult",
	elsePath->	ConstInt64((int64_t)falseObject));

	return INLINE_SUCCESSFUL_NO_GENERIC_PATH_REQUIRED;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForGenericNotNil(TR::BytecodeBuilder *builder)
{
	TR::IlValue *receiver = PICK(builder, 0);

	TR::IlBuilder *thenPath = nullptr;
	TR::IlBuilder *elsePath = nullptr;
	builder->IfThenElse(&thenPath, &elsePath,
	builder->	EqualTo(receiver,
	builder->		ConstInt64((int64_t)nilObject)));

	/* store true result */
	thenPath->Store("sendResult",
	thenPath->	ConstInt64((int64_t)falseObject));

	/* store false result */
	elsePath->Store("sendResult",
	elsePath->	ConstInt64((int64_t)trueObject));

	return INLINE_SUCCESSFUL_NO_GENERIC_PATH_REQUIRED;
}

void
SOMppMethod::generateForWhileLoop(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, long bytecodeIndex, int32_t recursiveLevel, TR::IlValue *condition)
{
	TR::BytecodeBuilder *loopBody = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	builder->AddSuccessorBuilder(&loopBody);
	TR::IlBuilder *looper = (TR::IlBuilder *)loopBody;

	TR::IlValue *self = getSelf(builder);

	long numOfLocals = whileLoopCodeBlock->GetNumberOfLocals();
	if (numOfLocals > 0) {
		builder->Store(getLocalName(recursiveLevel),
		builder->	CreateLocalArray((int32_t)numOfLocals, Int64));
	}

	TR::BytecodeBuilder *preStart = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	TR::BytecodeBuilder *preEnd = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));

	builder->AddSuccessorBuilder(&preStart);
	builder->AppendBuilder(preStart);
	builder->AddSuccessorBuilder(&preEnd);
	builder->AppendBuilder(preEnd);

	/* generate the initial loop condition. */
	generateGenericMethodBody(&preStart, genericSend, whileLoopConditionBlock, self, bytecodeIndex, recursiveLevel);

	preStart->Goto(preEnd);

	preEnd->Store("keepIterating",
	preEnd->	EqualTo(
	preEnd->		Load("sendResult"), condition));

	TR::BytecodeBuilder *first = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	TR::BytecodeBuilder *last = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));

	loopBody->AddSuccessorBuilder(&first);
	loopBody->AppendBuilder(first);
	loopBody->AddSuccessorBuilder(&last);
	loopBody->AppendBuilder(last);

	preEnd->WhileDoLoop("keepIterating", &looper);

	/* START OF LOOP */
#if SOM_METHOD_DEBUG
	fprintf(stderr, " %s>>#%s ", whileLoopCodeBlock->GetHolder()->GetName()->GetChars(), whileLoopCodeBlock->GetSignature()->GetChars());
#endif
	generateGenericMethodBody(&first, genericSend, whileLoopCodeBlock, self, bytecodeIndex, recursiveLevel);

	/* Generate code for the loop condition */
	generateGenericMethodBody(&first, genericSend, whileLoopConditionBlock, self, bytecodeIndex, recursiveLevel);
	first->Goto(last);

	last->Store("keepIterating",
	last->	EqualTo(
	last->		Load("sendResult"), condition));
	/* END OF LOOP */

	preEnd->Store("sendResult",
	preEnd->	ConstInt64((int64_t)nilObject));
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForWhileTrue(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, long bytecodeIndex, int32_t recursiveLevel)
{
	generateForWhileLoop(builder, genericSend, bytecodeIndex, recursiveLevel, builder->ConstInt64((int64_t)trueObject));
	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForWhileFalse(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, long bytecodeIndex, int32_t recursiveLevel)
{
	generateForWhileLoop(builder, genericSend, bytecodeIndex, recursiveLevel, builder->ConstInt64((int64_t)falseObject));
	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForBooleanAnd(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, long bytecodeIndex)
{
	TR::IlValue *receiver = PICK(builder, 1);
	TR::IlValue *param1 = PICK(builder, 0);

	verifyBooleanArg(builder, genericSend, param1);

	TR::IlBuilder *thenPath = nullptr;
	TR::IlBuilder *elsePath = nullptr;
	builder->IfThenElse(&thenPath, &elsePath,
	builder->	EqualTo(
	builder->		ConstInt64((int64_t)trueObject), receiver));

	thenPath->Store("sendResult", param1);

	elsePath->Store("sendResult",
	elsePath->	ConstInt64((int64_t)falseObject));

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForBooleanOr(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, long bytecodeIndex)
{
	TR::IlValue *receiver = PICK(builder, 1);
	TR::IlValue *param1 = PICK(builder, 0);

	verifyBooleanArg(builder, genericSend, param1);

	TR::IlBuilder *thenPath = nullptr;
	TR::IlBuilder *elsePath = nullptr;
	builder->IfThenElse(&thenPath, &elsePath,
	builder->	EqualTo(
	builder->		ConstInt64((int64_t)trueObject), receiver));

	thenPath->Store("sendResult", receiver);

	elsePath->Store("sendResult", param1);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineOSRToGenericSend(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, long bytecodeIndex)
{
	TR::BytecodeBuilder *failurePath = nullptr;
	builder->IfCmpEqualZero(&failurePath,
	builder->		ConstInt64(0x0));

	SET_STACKTOP(failurePath, stackTopForErrorHandling);
	failurePath->Goto(*genericSend);

	return INLINE_SUCCESSFUL;
}

bool
SOMppMethod::methodIsInlineable(VMMethod *vmMethod, int32_t recursiveLevel)
{
	long bytecodeCount = vmMethod->GetNumberOfBytecodes();
	long i = 0;
	bool canInline = true;

	if (recursiveLevel > 4) {
		return false;
	}

#if SOM_METHOD_DEBUG
	fprintf(stderr, "\n\t\tmethodIsInlineable %s\n", vmMethod->GetSignature()->GetChars());
#endif

	while ((i < bytecodeCount) && canInline) {
		uint8_t bc = vmMethod->GetBytecode(i);
#if SOM_METHOD_DEBUG
		fprintf(stderr, "\t\t\tinline %d %s", i, Bytecode::GetBytecodeName(bc));
#endif
		switch(bc) {
		case BC_DUP:
		case BC_PUSH_FIELD:
		case BC_PUSH_CONSTANT:
		case BC_PUSH_GLOBAL:
		case BC_POP:
		case BC_POP_ARGUMENT:
		case BC_POP_FIELD:
			break;
		case BC_JUMP_IF_FALSE:
		case BC_JUMP_IF_TRUE:
		case BC_JUMP:
		{
#if SOM_METHOD_DEBUG
			fprintf(stderr, " %d ", calculateBytecodeIndexForJump(vmMethod, i));
#endif
			break;
		}
		case BC_RETURN_LOCAL:
			break;
		case BC_RETURN_NON_LOCAL:
		{
			if (recursiveLevel > 0) {
				canInline = false;
			}
			break;
		}
		case BC_PUSH_ARGUMENT:
		{
			uint8_t level = vmMethod->GetBytecode(i + 2);
			if (level > 2) {
				canInline = false;
			}
			break;
		}
		case BC_PUSH_LOCAL:
		{
			uint8_t level = vmMethod->GetBytecode(i + 2);
			if (level > 2) {
				canInline = false;
			}
			break;
		}
		case BC_POP_LOCAL:
		{
			uint8_t level = vmMethod->GetBytecode(i + 2);
			if (level > 2) {
				canInline = false;
			}
			break;
		}
		case BC_SEND:
		{
			VMSymbol* signature = static_cast<VMSymbol*>(vmMethod->GetConstant(i));
#if SOM_METHOD_DEBUG
			fprintf(stderr, " %s", signature->GetChars());
#endif
			VMClass *receiverFromCache = vmMethod->getInvokeReceiverCache(i);
			if (nullptr != receiverFromCache) {
				VMClass *invokableClass = receiverFromCache->LookupInvokable(signature)->GetHolder();
				SOMppMethod::RECOGNIZED_METHOD_INDEX recognizedMethodIndex = getRecognizedMethodIndex(vmMethod, receiverFromCache, invokableClass, signature->GetChars(), recursiveLevel + 1);
				if (NOT_RECOGNIZED == recognizedMethodIndex) {
					VMInvokable *invokable = receiverFromCache->LookupInvokable(signature);
					if (nullptr == invokable) {
						canInline = false;
					} else if (!invokable->IsPrimitive()) {
						VMMethod *methodToInline = static_cast<VMMethod*>(invokable);
						if (!methodIsInlineable(methodToInline, recursiveLevel + 1)) {
							canInline = false;
						}
					} else {
						canInline = false;
					}
				}
			}
			/* If receiverFromCache is NULL generate an OSR point back to the generic send of the method which is inling this method */
			break;
		}
		default:
			canInline = false;
		}
#if SOM_METHOD_DEBUG
		fprintf(stderr, "\n");
#endif
		i += Bytecode::GetBytecodeLength(bc);
	}
#if SOM_METHOD_DEBUG
	if (canInline) {
		fprintf(stderr, "\t\tsuccessfully inlined\n");
	} else {
		fprintf(stderr, "\t\tfailed to inline\n");
	}
#endif
	return canInline;
}

TR::IlValue *
SOMppMethod::getIntegerValue(TR::IlBuilder *builder, TR::IlValue *object)
{
	TR::IlValue *value = nullptr;
#if USE_TAGGING
	/* All integers are verified to be tagged otherwise it would fall to the generic send */
	value =
	builder->ShiftR(object,
	builder->	ConstInt32(1));
#else
	/* Read embedded slot vtable slot + gcField */
	builder->Store("integerValueSlot",
	builder->	Add(object,
	builder->		ConstInt64((int64_t)(sizeof(int64_t)+sizeof(size_t)))));

	value =
	builder->LoadAt(pInt64,
	builder->	ConvertTo(pInt64,
	builder->		Load("integerValueSlot")));
#endif

	return value;
}

TR::IlValue *
SOMppMethod::newIntegerObjectForValue(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, TR::IlValue *value)
{
#if USE_TAGGING
	TR::BytecodeBuilder *failurePath = nullptr;
	builder->IfCmpLessThan(&failurePath, value,
	builder->	ConstInt64((int64_t)VMTAGGEDINTEGER_MIN));

	builder->IfCmpGreaterThan(&failurePath, value,
	builder->	ConstInt64((int64_t)VMTAGGEDINTEGER_MAX));

	SET_STACKTOP(failurePath, stackTopForErrorHandling);
	failurePath->Goto(*genericSend);

	builder->Store("shiftedValue",
	builder->	ShiftL(value,
	builder->		ConstInt32(1)));

	builder->Store("newInteger",
	builder->	Add(
	builder->		Load("shiftedValue"),
	builder->		ConstInt64(1)));
#else
	builder->Store("newInteger",
	builder->	Call("newInteger", 1, value));
#endif

	return builder->Load("newInteger");
}

TR::IlValue *
SOMppMethod::getDoubleValue(TR::IlBuilder *builder, TR::IlValue *object)
{
	builder->Store("doubleSlot",
	builder->	Add(object,
	builder->		ConstInt64((int64_t)(sizeof(int64_t)+sizeof(size_t)))));

	/* Read value */
	TR::IlValue *value =
	builder->LoadAt(pDouble,
	builder->	ConvertTo(pDouble,
	builder->		Load("doubleSlot")));

	return value;
}

TR::IlValue *
SOMppMethod::getDoubleValueFromDoubleOrInteger(TR::IlBuilder *builder, TR::IlValue *object, TR::IlValue *objectClass)
{
	TR::IlBuilder *isDouble = nullptr;
	TR::IlBuilder *notDouble = nullptr;
	builder->IfThenElse(&isDouble, &notDouble,
	builder->	EqualTo(objectClass, builder->ConstInt64((int64_t)doubleClass)));

	isDouble->Store("doubleValue", getDoubleValue(isDouble, object));

	notDouble->Store("doubleValue", notDouble->ConvertTo(Double, getIntegerValue(notDouble, object)));

	return builder->Load("doubleValue");
}

TR::IlValue *
SOMppMethod::getIndexableFieldSlot(TR::BytecodeBuilder *builder, TR::IlValue *array, TR::IlValue *index)
{
	TR::IlValue *indexValue = getIntegerValue(builder, index);
	TR::IlValue *numberOfFields = builder->LoadIndirect("VMObject", "numberOfFields", array);
	TR::IlValue *vmObjectSize = builder->ConstInt64(sizeof(VMObject));
	TR::IlValue *vmObjectPointerSize = builder->ConstInt64(sizeof(VMObject*));
	TR::IlValue *indexableIndex = builder->Add(indexValue, numberOfFields);

	TR::IlValue *actualIndex =
	builder->Sub(indexableIndex,
	builder->	ConstInt64(1));

	TR::IlValue *offset =
	builder->Add(vmObjectSize,
	builder->	Mul(actualIndex, vmObjectPointerSize));

	return builder->Add(array, offset);
}

SOMppMethod::RECOGNIZED_METHOD_INDEX
SOMppMethod::getRecognizedMethodIndex(VMMethod *sendingMethod, VMClass *receiverFromCache, VMClass *invokableClass, const char *signatureChars, int32_t recursiveLevel)
{
	if (NULL == receiverFromCache) {
		/* return an inline for an OSR point back to the calling generic send */
		return OSR_TO_GENERIC_SEND;
	} else if ((VMClass*)objectClass == invokableClass) {
		/* First try objectClass and nilClass.  If they can not inline try the other classes */
		if (0 == strcmp("<>", signatureChars)) {
			/* <> on object returns (= other) not.  object = returns == other. Can only
			 * generate the object <> inline code if the receiver does not override =
			 */
			if (receiverFromCache->LookupInvokable(GetUniverse()->SymbolForChars("="))->GetHolder() == (VMClass*)objectClass) {
				return OBJECT_NOTEQUAL;
			} else if ((VMClass*)integerClass == receiverFromCache) {
				/* Integer is known to override "=" so inline the Integer specific <> */
				return INTEGER_NOTEQUAL;
			}
			/* TODO Maybe add double <> here */
		} else if (0 == strcmp("~=", signatureChars)) {
			/* ~= on object returns (== other) not. Can only generate the object ~= inline code
			 * if the receiver does not override ==
			 */
			if (receiverFromCache->LookupInvokable(GetUniverse()->SymbolForChars("=="))->GetHolder() == (VMClass*)objectClass) {
				return OBJECT_NOTEQUAL;
			}
		} else if (0 == strcmp("=", signatureChars)) {
			/* = on object returns ==.  Can only generate the object = inline code if the receiver
			 * does not override =
			 */
			if (receiverFromCache->LookupInvokable(GetUniverse()->SymbolForChars("="))->GetHolder() == (VMClass*)objectClass) {
				return OBJECT_EQUAL;
			}
		} else if (0 == strcmp("==", signatureChars)) {
			return OBJECT_EQUAL;
		} else if (0 == strcmp("isNil", signatureChars)) {
			return GENERIC_ISNIL;
		} else if (0 == strcmp("notNil", signatureChars)) {
			return GENERIC_NOTNIL;
		} else if (0 == strcmp("value", signatureChars)) {
			return OBJECT_VALUE;
		}
	} else if ((VMClass*)nilClass == invokableClass) {
		if (0 == strcmp("isNil", signatureChars)) {
			return GENERIC_ISNIL;
		} else if (0 == strcmp("notNil", signatureChars)) {
			return GENERIC_NOTNIL;
		}
	} else if ((VMClass*)integerClass == invokableClass) {
		if (0 == strcmp("+", signatureChars)) {
			return INTEGER_PLUS;
		} else if (0 == strcmp("-", signatureChars)) {
			return INTEGER_MINUS;
		} else if (0 == strcmp("*", signatureChars)) {
			return INTEGER_MULTIPLY;
		} else if (0 == strcmp("/", signatureChars)) {
			return INTEGER_DIVIDE;
		} else if (0 == strcmp("%", signatureChars)) {
			return INTEGER_PERCENT;
		} else if (0 == strcmp("&", signatureChars)) {
			return INTEGER_AND;
		} else if (0 == strcmp("<=", signatureChars)) {
			return INTEGER_LESSTHANEQUAL;
		} else if (0 == strcmp("<", signatureChars)) {
			return INTEGER_LESSTHAN;
		} else if (0 == strcmp(">=", signatureChars)) {
			return INTEGER_GREATERTHANEQUAL;
		} else if (0 == strcmp(">", signatureChars)) {
			return INTEGER_GREATERTHAN;
		} else if (0 == strcmp("=", signatureChars)) {
			return INTEGER_EQUAL;
		} else if (0 == strcmp("~=", signatureChars)) {
			return INTEGER_NOTEQUAL;
		} else if (0 == strcmp("negated", signatureChars)){
			return INTEGER_NEGATED;
		} else if (0 == strcmp("max:", signatureChars)){
			return INTEGER_MAX;
		} else if (0 == strcmp("abs", signatureChars)){
			return INTEGER_ABS;
		} else if (0 == strcmp("to:by:do:", signatureChars)){
			if (0 != strcmp("to:do:", sendingMethod->GetSignature()->GetChars())) {
				forLoopBlock = blockMethods.top();
				blockMethods.pop();
				if (methodIsInlineable(forLoopBlock, recursiveLevel)) {
					return INTEGER_TOBYDO;
				}
			}
		} else if (0 == strcmp("to:do:", signatureChars)){
			forLoopBlock = blockMethods.top();
			blockMethods.pop();
			if (methodIsInlineable(forLoopBlock, recursiveLevel)) {
				return INTEGER_TODO;
			}
		} else if (0 == strcmp("downTo:by:do:", signatureChars)){
			if (0 != strcmp("downTo:do:", sendingMethod->GetSignature()->GetChars())) {
				forLoopBlock = blockMethods.top();
				blockMethods.pop();
				if (methodIsInlineable(forLoopBlock, recursiveLevel)) {
					return INTEGER_DOWNTOBYDO;
				}
			}
		} else if (0 == strcmp("downTo:do:", signatureChars)){
			forLoopBlock = blockMethods.top();
			blockMethods.pop();
			if (methodIsInlineable(forLoopBlock, recursiveLevel)) {
				return INTEGER_DOWNTODO;
			}
		}
	} else if ((VMClass*)arrayClass == invokableClass) {
		if (0 == strcmp("at:", signatureChars)) {
			return ARRAY_AT;
		} else if (0 == strcmp("at:put:", signatureChars)) {
			return ARRAY_ATPUT;
		} else if (0 == strcmp("length", signatureChars)) {
			return ARRAY_LENGTH;
		}
	} else if ((VMClass*)doubleClass == invokableClass) {
		 if (0 == strcmp("+", signatureChars)) {
			return DOUBLE_PLUS;
		} else if (0 == strcmp("-", signatureChars)) {
			return DOUBLE_MINUS;
		} else if (0 == strcmp("*", signatureChars)) {
			return DOUBLE_MULTIPLY;
		} else if (0 == strcmp("//", signatureChars)) {
			return DOUBLE_DIVIDE;
		} else if (0 == strcmp("<", signatureChars)) {
			return DOUBLE_LESSTHAN;
		} else if (0 == strcmp("<=", signatureChars)) {
			return DOUBLE_LESSTHANEQUAL;
		} else if (0 == strcmp(">", signatureChars)) {
			return DOUBLE_GREATERTHAN;
		} else if (0 == strcmp(">=", signatureChars)) {
			return DOUBLE_GREATERTHANEQUAL;
		}
	} else if (((VMClass*)blockClass == invokableClass) || ((VMClass*)blockClass == invokableClass->GetSuperClass())) {
		if (0 == strcmp("whileTrue:", signatureChars)) {
			char *sendMethodChars = sendingMethod->GetSignature()->GetChars();
			if ((0 != strcmp("whileFalse:", sendMethodChars)) && (0 != strcmp("to:by:do:", sendMethodChars)) && (0 != strcmp("downTo:by:do:", sendMethodChars))) {
				whileLoopCodeBlock = blockMethods.top();
				blockMethods.pop();
				whileLoopConditionBlock = blockMethods.top();
				blockMethods.pop();
				if (methodIsInlineable(whileLoopConditionBlock, recursiveLevel) && methodIsInlineable(whileLoopCodeBlock, recursiveLevel)) {
					return BLOCK_WHILETRUE;
				}
			}
		} else if (0 == strcmp("whileFalse:", signatureChars)) {
			whileLoopCodeBlock = blockMethods.top();
			blockMethods.pop();
			whileLoopConditionBlock = blockMethods.top();
			blockMethods.pop();
			if (methodIsInlineable(whileLoopConditionBlock, recursiveLevel) && methodIsInlineable(whileLoopCodeBlock, recursiveLevel)) {
				return BLOCK_WHILEFALSE;
			}
		}
	} else if (((VMClass*)trueClass == receiverFromCache) || ((VMClass*)falseClass == receiverFromCache)) {
		if ((0 == strcmp("&&", signatureChars)) || (0 == strcmp("and:", signatureChars))) {
			return BOOLEAN_AND;
		} else if ((0 == strcmp("||", signatureChars)) || (0 == strcmp("or:", signatureChars))) {
			return BOOLEAN_OR;
		}
	}
	return NOT_RECOGNIZED;
}
