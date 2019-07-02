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

#include "SOMppMethod_with_vm_state.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <dlfcn.h>
#include <errno.h>

#include "JitBuilder.hpp"
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

#define DO_DEBUG_PRINTS 0

#if DEBUG
#ifndef UNITTESTS
#define SOM_METHOD_DEBUG true
#endif
#elif 1 == DO_DEBUG_PRINTS
#define SOM_METHOD_DEBUG true
#else
#undef SOM_METHOD_DEBUG
#endif

class SOMppVMState : public OMR::JitBuilder::VirtualMachineState
   {
   public:
     SOMppVMState()
       : OMR::JitBuilder::VirtualMachineState(),
	 _stack(nullptr),
	 _stackTop(nullptr)
     {}
     
     SOMppVMState(OMR::JitBuilder::VirtualMachineOperandStack *stack, OMR::JitBuilder::VirtualMachineRegister *stackTop)
       : OMR::JitBuilder::VirtualMachineState(),
	 _stack(stack),
	 _stackTop(stackTop)
     {}

     void DropAll(OMR::JitBuilder::IlBuilder *b)
     {
       while(_stack->GetStackTop() > -1) {
	 _stack->Pop(b);
       }
     }
     
     virtual void Commit(OMR::JitBuilder::IlBuilder *b)
     {
       _stack->Commit(b);
       _stackTop->Commit(b);
     }

     virtual void Reload(OMR::JitBuilder::IlBuilder *b)
     {
       _stack->Reload(b);
       _stackTop->Reload(b);
     }

   virtual OMR::JitBuilder::VirtualMachineState *MakeCopy()
      {
	  SOMppVMState *newState = new SOMppVMState();
	  newState->_stack = (OMR::JitBuilder::VirtualMachineOperandStack *)_stack->MakeCopy();
	  newState->_stackTop = (OMR::JitBuilder::VirtualMachineRegister *) _stackTop->MakeCopy();
	  return newState;
      }

   virtual void MergeInto(OMR::JitBuilder::VirtualMachineState *other, OMR::JitBuilder::IlBuilder *b)
      {
	   SOMppVMState *otherState = (SOMppVMState *)other;
	   _stack->MergeInto(otherState->_stack, b);
	   _stackTop->MergeInto(otherState->_stackTop, b);
      }

   OMR::JitBuilder::VirtualMachineOperandStack * _stack;
   OMR::JitBuilder::VirtualMachineRegister     * _stackTop;
   };

#define STACK(b)	  (((SOMppVMState *)(b)->vmState())->_stack)
#define COMMIT(b)         ((b)->vmState()->Commit(b))
#define RELOAD(b)         ((b)->vmState()->Reload(b))
#define PUSH(b,v)	  (STACK(b)->Push(b,v))
#define POP(b)            (STACK(b)->Pop(b))
#define TOP(b)            (STACK(b)->Top())
#define DUP(b)            (STACK(b)->Dup(b))
#define DROP(b,d)         (STACK(b)->Drop(b,d))
#define DROPALL(b)        (((SOMppVMState *)(b)->vmState())->DropAll(b))
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

OMR::JitBuilder::IlValue *
SOMppMethod::add(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *param1, OMR::JitBuilder::IlValue *param2)
{
	return builder->Add(param1, param2);
}
OMR::JitBuilder::IlValue *
SOMppMethod::sub(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *param1, OMR::JitBuilder::IlValue *param2)
{
	return builder->Sub(param1, param2);
}
OMR::JitBuilder::IlValue *
SOMppMethod::mul(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *param1, OMR::JitBuilder::IlValue *param2)
{
	return builder->Mul(param1, param2);
}
OMR::JitBuilder::IlValue *
SOMppMethod::div(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *param1, OMR::JitBuilder::IlValue *param2)
{
	return builder->Div(param1, param2);
}
OMR::JitBuilder::IlValue *
SOMppMethod::percent(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *param1, OMR::JitBuilder::IlValue *param2)
{
	OMR::JitBuilder::IlValue *divResult = builder->Div(param1, param2);
	OMR::JitBuilder::IlValue *mulResult = builder->Mul(divResult, param2);

	builder->Store("subResult",
	builder->	Sub(param1, mulResult));

	OMR::JitBuilder::IlBuilder *greater = nullptr;
	builder->IfThen(&greater,
	builder->	GreaterThan(param1,
	builder->		ConstInt64(0)));

	OMR::JitBuilder::IlBuilder *lessThan = nullptr;
	greater->IfThen(&lessThan,
	greater->	LessThan(param1,
	greater->		ConstInt64(0)));

	lessThan->Store("subResult",
	lessThan->	Add(
	lessThan->		Load("subResult"), param2));

	return builder->Load("subResult");
}
OMR::JitBuilder::IlValue *
SOMppMethod::andVals(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *param1, OMR::JitBuilder::IlValue *param2)
{
	return builder->And(param1, param2);
}
OMR::JitBuilder::IlValue *
SOMppMethod::lessThan(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *param1, OMR::JitBuilder::IlValue *param2)
{
	return builder->LessThan(param1, param2);
}
OMR::JitBuilder::IlValue *
SOMppMethod::greaterThan(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *param1, OMR::JitBuilder::IlValue *param2)
{
	return builder->GreaterThan(param1, param2);
}
OMR::JitBuilder::IlValue *
SOMppMethod::equalTo(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *param1, OMR::JitBuilder::IlValue *param2)
{
	return builder->EqualTo(param1, param2);
}
void
SOMppMethod::forLoopUp(OMR::JitBuilder::BytecodeBuilder *builder, const char *index, OMR::JitBuilder::IlBuilder **loop, OMR::JitBuilder::IlValue *start, OMR::JitBuilder::IlValue *end, OMR::JitBuilder::IlValue *increment)
{
     builder->ForLoopUp(const_cast<char*>(index), loop, start, end, increment);
}

void
SOMppMethod::forLoopDown(OMR::JitBuilder::BytecodeBuilder *builder, const char *index, OMR::JitBuilder::IlBuilder **loop, OMR::JitBuilder::IlValue *start, OMR::JitBuilder::IlValue *end, OMR::JitBuilder::IlValue *increment)
{
     builder->ForLoopDown(const_cast<char*>(index), loop, start, end, increment);
}

SOMppMethod::SOMppMethod(OMR::JitBuilder::TypeDictionary *types, VMMethod *vmMethod, bool inlineCalls) :
		MethodBuilder(types),
		method(vmMethod),
		extraStackDepthRequired(0),
		doInlining(inlineCalls),
		doLoopInlining(inlineCalls)
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

	for (int i = 0; i < MAX_RECURSIVE_INLINING_DEPTH + 1; i++) {
		stackTopForErrorHandling[i] = 0;
		inlinedMethods[i] = nullptr;
		inlinedBytecodeIndecies[i] = 0;
	}

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
	DefineParameter("interpreter", pInt64);
	DefineParameter("frame", pVMFrame);
}

void
SOMppMethod::defineLocals()
{
	DefineLocal("frameOuterContext", pInt64);
	DefineLocal("frameArguments", pInt64);
	DefineLocal("frameLocals", pInt64);
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
	DefineFunction((char *)"getSuperClass", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::GET_SUPER_CLASS_LINE, (void *)&BytecodeHelper::getSuperClass, Int64, 1, Int64);
	DefineFunction((char *)"getGlobal", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::GET_GLOBAL_LINE, (void *)&BytecodeHelper::getGlobal, Int64, 1, Int64);
	DefineFunction((char *)"getNewBlock", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::GET_NEW_BLOCK_LINE, (void *)&BytecodeHelper::getNewBlock, Int64, 3, Int64, Int64, Int64);
	DefineFunction((char *)"newInteger", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::NEW_INTEGER_LINE, (void *)&BytecodeHelper::newInteger, Int64, 1, Int64);
	DefineFunction((char *)"newDouble", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::NEW_DOUBLE_LINE, (void *)&BytecodeHelper::newDouble, Int64, 1, Double);
	DefineFunction((char *)"newArray", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::NEW_ARRAY_LINE, (void *)&BytecodeHelper::newArray, Int64, 1, Int64);
	DefineFunction((char *)"getFieldFrom", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::GET_FIELD_FROM_LINE, (void *)&BytecodeHelper::getFieldFrom, Int64, 2, Int64, Int64);
	DefineFunction((char *)"setFieldTo", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::SET_FIELD_TO_LINE, (void *)&BytecodeHelper::setFieldTo, NoType, 3, Int64, Int64, Int64);
	DefineFunction((char *)"getInvokable", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::GET_INVOKABLE_LINE, (void *)&BytecodeHelper::getInvokable, Int64, 2, Int64, Int64);
	DefineFunction((char *)"doSendIfRequired", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::DO_SEND_IF_REQUIRED_LINE, (void *)&BytecodeHelper::doSendIfRequired, Int64, 6, Int64, Int64, Int64, Int64, Int64, Int64);
	DefineFunction((char *)"allocateVMFrame", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::ALLOCATE_VMFRAME_LINE, (void *)&BytecodeHelper::allocateVMFrame, Int64, 8, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64);
	DefineFunction((char *)"doInlineSendIfRequired", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::DO_INLINE_SEND_IF_REQUIRED_LINE, (void *)&BytecodeHelper::doInlineSendIfRequired, Int64, 6, Int64, Int64, Int64, Int64, Int64, Int64);
	DefineFunction((char *)"doSuperSendHelper", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::DO_SUPER_SEND_HELPER_LINE, (void *)&BytecodeHelper::doSuperSendHelper, Int64, 4, Int64, Int64, Int64, Int64);
	DefineFunction((char *)"popFrameAndPushResult", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::POP_FRAME_AND_PUSH_RESULT_LINE, (void *)&BytecodeHelper::popFrameAndPushResult, NoType, 3, Int64, Int64, Int64);
	DefineFunction((char *)"popToContext", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::POP_TO_CONTEXT_LINE, (void *)&BytecodeHelper::popToContext, Int64, 2, Int64, Int64);
	DefineFunction((char *)"printObject", (char *)BytecodeHelper::BYTECODEHELPER_FILE, (char *)BytecodeHelper::PRINT_OBJECT_LINE, (void *)&BytecodeHelper::printObject, Int64, 3, Int64, Int64, Int64);
}

void
SOMppMethod::defineVMFrameStructure(OMR::JitBuilder::TypeDictionary *types)
{
	vmFrame = types->DefineStruct("VMFrame");

	pVMFrame = types->PointerTo("VMFrame");

	types->DefineField("VMFrame", "vTable", pInt64);
	types->DefineField("VMFrame", "gcField", pInt64);
	types->DefineField("VMFrame", "hash", Int64);
	types->DefineField("VMFrame", "objectSize", Int64);
	types->DefineField("VMFrame", "numberOfFields", Int64);
	types->DefineField("VMFrame", "clazz", pInt64);
	types->DefineField("VMFrame", "previousFrame", pInt64);
	types->DefineField("VMFrame", "context", pInt64);
	types->DefineField("VMFrame", "method", pInt64);
	types->DefineField("VMFrame", "isJITFrame", Int64);
	types->DefineField("VMFrame", "bytecodeIndex", Int64);
	types->DefineField("VMFrame", "arguments", pInt64);
	types->DefineField("VMFrame", "locals", pInt64);
	types->DefineField("VMFrame", "stack_ptr", pInt64);
	
	types->CloseStruct("VMFrame");
}

void
SOMppMethod::defineVMObjectStructure(OMR::JitBuilder::TypeDictionary *types)
{
	vmObject = types->DefineStruct("VMObject");

	types->DefineField("VMObject", "vTable", pInt64);
	types->DefineField("VMObject", "gcField", pInt64);
	types->DefineField("VMObject", "hash", Int64);
	types->DefineField("VMObject", "objectSize", Int64);
	types->DefineField("VMObject", "numberOfFields", Int64);
	types->DefineField("VMObject", "clazz", pInt64);

	for (int i = 0; i < FIELDNAMES_LENGTH; i++) {
		types->DefineField("VMObject", fieldNames[i], pInt64);
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

OMR::JitBuilder::BytecodeBuilder **
SOMppMethod::createBytecodesBuilder(VMMethod *vmMethod)
{
	long numOfBytecodes = vmMethod->GetNumberOfBytecodes();
	long tableSize = sizeof(OMR::JitBuilder::BytecodeBuilder *) * numOfBytecodes;
	OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable = (OMR::JitBuilder::BytecodeBuilder **)malloc(tableSize);
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
SOMppMethod::justReturn(OMR::JitBuilder::IlBuilder *from)
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

	if (NOT_RECOGNIZED != getRecognizedMethodIndex(method, method->GetHolder(), method->GetHolder(), method->GetSignature()->GetChars(), 0, false)) {
		/* Do not inline loops into recognized methods when they are being compiled */
		doLoopInlining = false;
	}

	OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable = createBytecodesBuilder(method);
	if (nullptr == bytecodeBuilderTable) {
		return false;
	}

//	if (0 != strcmp("MyTest>>#sum", methodName)) {
//		return false;
//	}

#if SOM_METHOD_DEBUG
	fprintf(stderr, "\nGenerating code for %s\n", methodName);
#endif

	stackTop = new OMR::JitBuilder::VirtualMachineRegisterInStruct(this, "VMFrame", "frame", "stack_ptr", "SP");
	stack = new OMR::JitBuilder::VirtualMachineOperandStack(this, 32, valueType, stackTop);

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

//	if (canHandle) {
//		if (extraStackDepthRequired > 0) {
//			method->SetMaximumNumberOfStackElements((uint8_t)extraStackDepthRequired + method->GetMaximumNumberOfStackElements());
//		}
//	}
	fprintf(stderr, "finished generating\n");
	return canHandle;
}

bool SOMppMethod::generateILForBytecodes(VMMethod *vmMethod, OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable)
{
	long numOfBytecodes = method->GetNumberOfBytecodes();
	int32_t bytecodeIndex = GetNextBytecodeFromWorklist();
	bool canHandle = true;

	while (canHandle && (-1 != bytecodeIndex)) {
		uint8_t bytecode = vmMethod->GetBytecode(bytecodeIndex);
		int32_t nextBCIndex = bytecodeIndex + Bytecode::GetBytecodeLength(bytecode);
		OMR::JitBuilder::BytecodeBuilder *builder = bytecodeBuilderTable[bytecodeIndex];
		OMR::JitBuilder::BytecodeBuilder *fallThroughBuilder = nullptr;

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
SOMppMethod::doDup(OMR::JitBuilder::BytecodeBuilder *builder)
{
	DUP(builder);
}

void
SOMppMethod::doPushLocal(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex)
{
	uint8_t index = method->GetBytecode(bytecodeIndex + 1);
	uint8_t level = method->GetBytecode(bytecodeIndex + 2);
#if SOM_METHOD_DEBUG
	fprintf(stderr, " %d %d ", index, level);
#endif
	OMR::JitBuilder::IlValue *locals = nullptr;
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
SOMppMethod::doPushArgument(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex)
{
	uint8_t index = method->GetBytecode(bytecodeIndex + 1);
	uint8_t level = method->GetBytecode(bytecodeIndex + 2);
#if SOM_METHOD_DEBUG
	fprintf(stderr, " %d %d ", index, level);
#endif
	OMR::JitBuilder::IlValue *arguments = nullptr;
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
SOMppMethod::doPushField(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex)
{
	OMR::JitBuilder::IlValue *object = getSelf(builder);
#if SOM_METHOD_DEBUG
	fprintf(stderr, " %d ", method->GetBytecode(bytecodeIndex + 1));
#endif
	pushField(builder, object, method->GetBytecode(bytecodeIndex + 1));
}

void
SOMppMethod::doPushBlock(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex)
{
	/* TODO come back and handle optimization in Interpreter::doPushBlock */
	VMMethod *blockMethod = static_cast<VMMethod*>(method->GetConstant(bytecodeIndex));
	long numOfArgs = blockMethod->GetNumberOfArguments();

	blockMethods.push(blockMethod);

#if SOM_METHOD_DEBUG
	fprintf(stderr, " %p ", blockMethod);
#endif

	OMR::JitBuilder::IlValue *block =
	builder->Call("getNewBlock", 3,
	builder->	Load("frame"),
	builder->	ConstInt64((int64_t)blockMethod),
	builder->	ConstInt64((int64_t)numOfArgs));

	OMR::JitBuilder::IlValue *self = getSelf(builder);

	blockToReceiverMap.insert(std::make_pair((const void *)block, self));
//	if (ret.second == false) {
//		fprintf(stderr, " hmmmmmm ");
//	} else {
//		fprintf(stderr, " added block %p with self %p  ret %p which was key %p value %p  ", block, self, ret.first, ret.first->first, ret.first->second);
//	}

	PUSH(builder, block);
}

void
SOMppMethod::doPushConstant(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex)
{
#if SOM_METHOD_DEBUG
	fprintf(stderr, " %d ", method->GetBytecode(bytecodeIndex + 1));
#endif
	pushConstant(builder, method, method->GetBytecode(bytecodeIndex + 1));
}

void
SOMppMethod::doPushGlobal(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex)
{
	pushGlobal(builder, static_cast<VMSymbol*>(method->GetConstant(bytecodeIndex)));
}

void
SOMppMethod::doPop(OMR::JitBuilder::BytecodeBuilder *builder)
{
	POP(builder);
}

void
SOMppMethod::doPopLocal(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex)
{
	uint8_t index = method->GetBytecode(bytecodeIndex + 1);
	uint8_t level = method->GetBytecode(bytecodeIndex + 2);
#if SOM_METHOD_DEBUG
	fprintf(stderr, " %d %d ", index, level);
#endif
	OMR::JitBuilder::IlValue *locals = nullptr;
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
SOMppMethod::doPopArgument(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex)
{
	/* see Interpreter::doPopArgument and VMFrame::SetArgument.
	 * level does not appear to be used. */
	popValueToArray(builder, builder->Load("frameArguments"), method->GetBytecode(bytecodeIndex + 1));
}

void
SOMppMethod::doPopField(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex)
{
#if SOM_METHOD_DEBUG
	fprintf(stderr, " %d ", method->GetBytecode(bytecodeIndex + 1));
#endif
	popField(builder, getSelf(builder), method->GetBytecode(bytecodeIndex + 1));
}

void
SOMppMethod::doSend(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex, OMR::JitBuilder::BytecodeBuilder *fallThrough)
{
	VMSymbol* signature = static_cast<VMSymbol*>(method->GetConstant(bytecodeIndex));
	int numOfArgs = Signature::GetNumberOfArguments(signature);
	OMR::JitBuilder::BytecodeBuilder *genericSend = nullptr;
	OMR::JitBuilder::BytecodeBuilder *merge = nullptr;

#if SOM_METHOD_DEBUG
	fprintf(stderr, " %s ", signature->GetChars());
#endif

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
		genericSend->	Call("doSendIfRequired", 6,
		genericSend->		Load("interpreter"),
		genericSend->		Load("frame"),
		genericSend->		Load("invokable"),
							PICK(genericSend, numOfArgs - 1),
		genericSend->		ConstInt64((int64_t)signature),
		genericSend->		ConstInt64((int64_t)bytecodeIndex)));

		OMR::JitBuilder::IlBuilder *bail = nullptr;
		genericSend->IfThen(&bail,
		genericSend->	EqualTo(
		genericSend->		Load("return"),
		genericSend->		ConstInt64(-1)));

		justReturn(bail);

		genericSend->Store("sendResult",
		genericSend->	LoadAt(pInt64,
		genericSend->		LoadIndirect("VMFrame", "stack_ptr",
		genericSend->			Load("frame"))));

		OMR::JitBuilder::BytecodeBuilder *restartIfRequired = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
		genericSend->IfCmpNotEqual(&restartIfRequired,
		genericSend->	Load("return"),
		genericSend->	ConstInt64((int64_t)bytecodeIndex));

		DROPALL(restartIfRequired);

		OMR::JitBuilder::BytecodeBuilder *start = bytecodeBuilderTable[0];
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
SOMppMethod::doSuperSend(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex)
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

	OMR::JitBuilder::IlBuilder *bail = nullptr;
	builder->IfThen(&bail,
	builder->   EqualTo(
	builder->		Load("return"),
	builder->		ConstInt64(-1)));

	justReturn(bail);

	DROP(builder, numOfArgs);

	OMR::JitBuilder::BytecodeBuilder *restartIfRequired = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	builder->IfCmpNotEqual(&restartIfRequired,
	builder->	Load("return"),
	builder->	ConstInt64((int64_t)bytecodeIndex));

	DROPALL(restartIfRequired);

	OMR::JitBuilder::BytecodeBuilder *start = bytecodeBuilderTable[0];
	restartIfRequired->Goto(start);

	OMR::JitBuilder::IlValue *value =
	builder->LoadAt(pInt64,
	builder->	LoadIndirect("VMFrame", "stack_ptr",
	builder->		Load("frame")));

	PUSH(builder, value);
}

void
SOMppMethod::doReturnLocal(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex)
{
	OMR::JitBuilder::IlValue *result = POP(builder);

	/* there is no need to commit the stack as it is not used */

	builder->Call("popFrameAndPushResult", 3,
	builder->	Load("interpreter"),
	builder->	Load("frame"), result);

	justReturn(builder);
}

void
SOMppMethod::doReturnNonLocal(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex)
{
	OMR::JitBuilder::IlValue *result = POP(builder);

	builder->Store("return",
	builder->	Call("popToContext", 2,
	builder->		Load("interpreter"),
	builder->		Load("frame")));

	OMR::JitBuilder::IlBuilder *continuePath = nullptr;
	OMR::JitBuilder::IlBuilder *didEscapedSend = nullptr;

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
SOMppMethod::doJumpIfFalse(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex)
{
#if SOM_METHOD_DEBUG
	fprintf(stderr, " jump to %lu ", calculateBytecodeIndexForJump(method, bytecodeIndex));
#endif
	OMR::JitBuilder::IlValue *value = POP(builder);
	OMR::JitBuilder::BytecodeBuilder *destBuilder = bytecodeBuilderTable[calculateBytecodeIndexForJump(method, bytecodeIndex)];
	builder->IfCmpEqual(&destBuilder, value,
	builder->	ConstInt64((int64_t)falseObject));
}

void
SOMppMethod::doJumpIfTrue(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex)
{
#if SOM_METHOD_DEBUG
	fprintf(stderr, " jump to %lu ", calculateBytecodeIndexForJump(method, bytecodeIndex));
#endif
	OMR::JitBuilder::IlValue *value = POP(builder);
	OMR::JitBuilder::BytecodeBuilder *destBuilder = bytecodeBuilderTable[calculateBytecodeIndexForJump(method, bytecodeIndex)];
	builder->IfCmpEqual(&destBuilder, value,
	builder->	ConstInt64((int64_t)trueObject));
}

void
SOMppMethod::doJump(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex)
{
#if SOM_METHOD_DEBUG
	fprintf(stderr, " jump to %lu ", calculateBytecodeIndexForJump(method, bytecodeIndex));
#endif
	OMR::JitBuilder::BytecodeBuilder *destBuilder = bytecodeBuilderTable[calculateBytecodeIndexForJump(method, bytecodeIndex)];
	builder->Goto(destBuilder);
}

void
SOMppMethod::pushValueFromArray(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *array, uint8_t arrayIndex)
{
	OMR::JitBuilder::IlValue *value =
	builder->LoadAt(pInt64,
	builder->	IndexAt(pInt64,
	builder->		ConvertTo(pInt64, array),
	builder->		ConstInt64(arrayIndex)));

	PUSH(builder, value);
}

void
SOMppMethod::pushField(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *object, uint8_t fieldIndex)
{
	if (fieldIndex < FIELDNAMES_LENGTH) {
		const char *fieldName = fieldNames[fieldIndex];
		PUSH(builder, builder->LoadIndirect("VMObject", fieldName, object));
	} else {
		OMR::JitBuilder::IlValue *field =
		builder->Call("getFieldFrom", 2, object,
		builder->	ConstInt64((int64_t)fieldIndex));
		PUSH(builder, field);
	}
}

void
SOMppMethod::pushConstant(OMR::JitBuilder::BytecodeBuilder *builder, VMMethod *vmMethod, uint8_t constantIndex)
{
	PUSH(builder, builder->ConstInt64((int64_t)vmMethod->indexableFields[constantIndex]));
}

void
SOMppMethod::pushGlobal(OMR::JitBuilder::BytecodeBuilder *builder, VMSymbol* globalName)
{
	OMR::JitBuilder::IlValue *global =
	builder->	Call("getGlobal", 1,
	builder->		ConstInt64((int64_t)globalName));

	OMR::JitBuilder::IlBuilder *globalIsNullPtr = nullptr;
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
SOMppMethod::popValueToArray(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *array, uint8_t arrayIndex)
{
	OMR::JitBuilder::IlValue *value = POP(builder);
	builder->StoreAt(
	builder->	IndexAt(pInt64,
	builder->		ConvertTo(pInt64, array),
	builder->		ConstInt64(arrayIndex)), value);
}

void
SOMppMethod::popField(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *object, uint8_t fieldIndex)
{
	OMR::JitBuilder::IlValue *value = POP(builder);
	if (fieldIndex < FIELDNAMES_LENGTH) {
		const char *fieldName = fieldNames[fieldIndex];
		builder->StoreIndirect("VMObject", fieldName, object, value);
	} else {
		builder->Call("setFieldTo", 3, object,
		builder->	ConstInt64((int64_t)fieldIndex), value);
	}
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
			OMR::JitBuilder::IlBuilder *iloop = nullptr;
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
SOMppMethod::getSelf(OMR::JitBuilder::IlBuilder *builder)
{
	OMR::JitBuilder::IlValue *context = builder->Load("frameOuterContext");
	OMR::JitBuilder::IlValue *self =
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

static const char *getSelfName(int64_t recursiveLevel)
{
	if (recursiveLevel == 0) {
		return "_self0";
	} else if (recursiveLevel == 1) {
		return "_self1";
	} else if (recursiveLevel == 2) {
		return "_self2";
	} else if (recursiveLevel == 3) {
		return "_self3";
	} else if (recursiveLevel == 4) {
		return "_self4";
	} else {
		return nullptr;
	}
}

SOMppMethod::INLINE_STATUS
SOMppMethod::doInlineIfPossible(OMR::JitBuilder::BytecodeBuilder **builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **merge, VMSymbol *signature, long bytecodeIndex)
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
				stackTopForErrorHandling[0] = GET_STACKTOP(*builder);
				if (NOT_RECOGNIZED != recognizedMethodIndex) {
					createBuildersForInlineSends(genericSend, merge, bytecodeIndex);
					OMR::JitBuilder::IlBuilder *b = *builder;
//					b->Store(getSelfName(0), getSelf(b));
					status = generateRecognizedMethod(*builder, genericSend, merge, recognizedMethodIndex, receiverFromCache, bytecodeIndex, 0);
				} else {
					if (!invokable->IsPrimitive()) {
						VMMethod *methodToInline = static_cast<VMMethod*>(invokable);
						if (methodIsInlineable(methodToInline, 0)) {
							createBuildersForInlineSends(genericSend, merge, bytecodeIndex);
							generateGenericMethod(builder, genericSend, merge, invokable, receiverFromCache, signature, bytecodeIndex, 0);
							status = INLINE_SUCCESSFUL;
						}
					} else {
						fprintf(stderr, "failed to inline due to primitive invokable %s:>>%s\n", receiverFromCache->GetName()->GetChars(), invokable->GetSignature()->GetChars());
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
SOMppMethod::generateRecognizedMethod(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, SOMppMethod::RECOGNIZED_METHOD_INDEX recognizedMethodIndex, VMClass *receiverFromCache, long bytecodeIndex, int32_t recursiveLevel)
{
	INLINE_STATUS status = INLINE_STATUS::INLINE_FAILED;

	switch(recognizedMethodIndex) {
	case OBJECT_EQUAL:
		status = generateInlineForObjectEqual(builder, genericSend, receiverFromCache, recursiveLevel);
		break;
	case OBJECT_NOTEQUAL:
		status = generateInlineForObjectNotEqual(builder, genericSend, receiverFromCache, recursiveLevel);
		break;
	case OBJECT_VALUE:
		status = generateInlineForObjectValue(builder, genericSend, receiverFromCache, recursiveLevel);
		break;
	case GENERIC_ISNIL:
		status = generateInlineForGenericIsNil(builder);
		break;
	case GENERIC_NOTNIL:
		status = generateInlineForGenericNotNil(builder);
		break;
	case INTEGER_PLUS:
		status = generateInlineForIntegerMath(builder, genericSend, &SOMppMethod::add, recursiveLevel);
		break;
	case INTEGER_MINUS:
		status = generateInlineForIntegerMath(builder, genericSend, &SOMppMethod::sub, recursiveLevel);
		break;
	case INTEGER_MULTIPLY:
		status = generateInlineForIntegerMath(builder, genericSend, &SOMppMethod::mul, recursiveLevel);
		break;
	case INTEGER_DIVIDE:
		status = generateInlineForIntegerMath(builder, genericSend, &SOMppMethod::div, recursiveLevel);
		break;
	case INTEGER_PERCENT:
		status = generateInlineForIntegerMath(builder, genericSend, &SOMppMethod::percent, recursiveLevel);
		break;
	case INTEGER_AND:
		status = generateInlineForIntegerMath(builder, genericSend, &SOMppMethod::andVals, recursiveLevel);
		break;
	case INTEGER_LESSTHAN:
		status = generateInlineForIntegerBoolean(builder, genericSend, &SOMppMethod::lessThan, builder->ConstInt64((int64_t)trueObject), builder->ConstInt64((int64_t)falseObject), recursiveLevel);
		break;
	case INTEGER_LESSTHANEQUAL:
		status = generateInlineForIntegerBoolean(builder, genericSend, &SOMppMethod::greaterThan, builder->ConstInt64((int64_t)falseObject), builder->ConstInt64((int64_t)trueObject), recursiveLevel);
		break;
	case INTEGER_GREATERTHAN:
		status = generateInlineForIntegerBoolean(builder, genericSend, &SOMppMethod::greaterThan, builder->ConstInt64((int64_t)trueObject), builder->ConstInt64((int64_t)falseObject), recursiveLevel);
		break;
	case INTEGER_GREATERTHANEQUAL:
		status = generateInlineForIntegerBoolean(builder, genericSend, &SOMppMethod::lessThan, builder->ConstInt64((int64_t)falseObject), builder->ConstInt64((int64_t)trueObject), recursiveLevel);
		break;
	case INTEGER_EQUAL:
		status = generateInlineForIntegerBoolean(builder, genericSend, &SOMppMethod::equalTo, builder->ConstInt64((int64_t)trueObject), builder->ConstInt64((int64_t)falseObject), recursiveLevel);
		break;
	case INTEGER_NOTEQUAL:
		status = generateInlineForIntegerBoolean(builder, genericSend, &SOMppMethod::equalTo, builder->ConstInt64((int64_t)falseObject), builder->ConstInt64((int64_t)trueObject), recursiveLevel);
		break;
	case INTEGER_NEGATED:
		status = generateInlineForIntegerNegated(builder, genericSend, recursiveLevel);
		break;
	case INTEGER_MAX:
		status = generateInlineForIntegerMax(builder, genericSend, recursiveLevel);
		break;
	case INTEGER_ABS:
		status = generateInlineForIntegerAbs(builder, genericSend, bytecodeIndex, recursiveLevel);
		break;
	case INTEGER_TODO:
		status = generateInlineForIntegerToDo(builder, genericSend, mergeSend, bytecodeIndex, recursiveLevel);
		break;
	case INTEGER_TOBYDO:
		status = generateInlineForIntegerToByDo(builder, genericSend, mergeSend, bytecodeIndex, recursiveLevel);
		break;
	case INTEGER_DOWNTODO:
		status = generateInlineForIntegerDownToDo(builder, genericSend, mergeSend, bytecodeIndex, recursiveLevel);
		break;
	case INTEGER_DOWNTOBYDO:
		status = generateInlineForIntegerDownToByDo(builder, genericSend, mergeSend, bytecodeIndex, recursiveLevel);
		break;
	case ARRAY_AT:
		status = generateInlineForArrayAt(builder, genericSend, recursiveLevel);
		break;
	case ARRAY_ATPUT:
		status = generateInlineForArrayAtPut(builder, genericSend, recursiveLevel);
		break;
	case ARRAY_LENGTH:
		status = generateInlineForArrayLength(builder, genericSend, recursiveLevel);
		break;
	case ARRAY_DO:
		status = generateInlineForArrayDo(builder, genericSend, mergeSend, bytecodeIndex, recursiveLevel);
		break;
	case ARRAY_DOINDEXES:
		status = generateInlineForArrayDoIndexes(builder, genericSend, mergeSend, bytecodeIndex, recursiveLevel);
		break;
	case ARRAY_NEW:
		status = generateInlineForArrayNew(builder, genericSend, mergeSend, bytecodeIndex, recursiveLevel);
		break;
	case DOUBLE_PLUS:
		status = generateInlineForDoubleMath(builder, genericSend, &SOMppMethod::add, recursiveLevel);
		break;
	case DOUBLE_MINUS:
		status = generateInlineForDoubleMath(builder, genericSend, &SOMppMethod::sub, recursiveLevel);
		break;
	case DOUBLE_MULTIPLY:
		status = generateInlineForDoubleMath(builder, genericSend, &SOMppMethod::mul, recursiveLevel);
		break;
	case DOUBLE_DIVIDE:
		status = generateInlineForDoubleMath(builder, genericSend, &SOMppMethod::div, recursiveLevel);
		break;
	case DOUBLE_LESSTHAN:
		status = generateInlineForDoubleBoolean(builder, genericSend, &SOMppMethod::lessThan, builder->ConstInt64((int64_t)trueObject), builder->ConstInt64((int64_t)falseObject), recursiveLevel);
		break;
	case DOUBLE_LESSTHANEQUAL:
		status = generateInlineForDoubleBoolean(builder, genericSend, &SOMppMethod::greaterThan, builder->ConstInt64((int64_t)falseObject), builder->ConstInt64((int64_t)trueObject), recursiveLevel);
		break;
	case DOUBLE_GREATERTHAN:
		status = generateInlineForDoubleBoolean(builder, genericSend, &SOMppMethod::greaterThan, builder->ConstInt64((int64_t)trueObject), builder->ConstInt64((int64_t)falseObject), recursiveLevel);
		break;
	case DOUBLE_GREATERTHANEQUAL:
		status = generateInlineForDoubleBoolean(builder, genericSend, &SOMppMethod::lessThan, builder->ConstInt64((int64_t)falseObject), builder->ConstInt64((int64_t)trueObject), recursiveLevel);
		break;
	case BLOCK_WHILETRUE:
		status = generateInlineForWhileTrue(builder, genericSend, mergeSend, bytecodeIndex, recursiveLevel);
		break;
	case BLOCK_WHILEFALSE:
		status = generateInlineForWhileFalse(builder, genericSend, mergeSend, bytecodeIndex, recursiveLevel);
		break;
	case BOOLEAN_AND:
		status = generateInlineForBooleanAnd(builder, genericSend, mergeSend, bytecodeIndex, recursiveLevel);
		break;
	case BOOLEAN_AND_NOBLOCK:
		status = generateInlineForBooleanAndNoBlock(builder, genericSend, mergeSend, bytecodeIndex, recursiveLevel);
		break;
	case BOOLEAN_OR:
		status = generateInlineForBooleanOr(builder, genericSend, mergeSend, bytecodeIndex, recursiveLevel);
		break;
	case BOOLEAN_OR_NOBLOCK:
		status = generateInlineForBooleanOrNoBlock(builder, genericSend, mergeSend, bytecodeIndex, recursiveLevel);
		break;
	case BOOLEAN_NOT:
		status = generateInlineForBooleanNot(builder, genericSend, mergeSend, bytecodeIndex, recursiveLevel);
		break;
	case OSR_TO_GENERIC_SEND:
		status = generateInlineOSRToGenericSend(builder, genericSend, bytecodeIndex, recursiveLevel);
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
OMR::JitBuilder::IlValue *
SOMppMethod::getLocalArrayForLevel(OMR::JitBuilder::BytecodeBuilder *builder, VMMethod *vmMethod, long bytecodeIndex, int32_t recursiveLevel)
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
			return builder->Load("frameLocals");
		} else {
			return builder->Load("frameContextLocals");
		}
	}
	return nullptr;
}

/* Only used for INLINING */
OMR::JitBuilder::IlValue *
SOMppMethod::getArgumentArrayForLevel(OMR::JitBuilder::BytecodeBuilder *builder, VMMethod *vmMethod, long bytecodeIndex, int32_t recursiveLevel)
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
SOMppMethod::generateGenericMethodBody(OMR::JitBuilder::BytecodeBuilder **ibuilder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **sendMerge, VMMethod *methodToInline, OMR::JitBuilder::IlValue *receiver, long bytecodeIndex, int32_t recursiveLevel)
{
	long bytecodeCount = methodToInline->GetNumberOfBytecodes();
	long bcIndex = 0;
	long tableSize = sizeof(OMR::JitBuilder::BytecodeBuilder *) * bytecodeCount;
	OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable = (OMR::JitBuilder::BytecodeBuilder **)malloc(tableSize);
	if (nullptr != bytecodeBuilderTable) {
		memset(bytecodeBuilderTable, 0, tableSize);
		long i = 0;
		while (i < bytecodeCount) {
			uint8_t bc = methodToInline->GetBytecode(i);
			bytecodeBuilderTable[i] = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
			i += Bytecode::GetBytecodeLength(bc);
		}
	} else {
		int *x = 0;
		fprintf(stderr, "failed to allocate bytecode builder table\n");
		*x = 0;
	}
	int* bytecodeTableEntryHasBeenReached = (int*)malloc(sizeof(int) * bytecodeCount);
	if (nullptr != bytecodeTableEntryHasBeenReached) {
		memset(bytecodeTableEntryHasBeenReached, 0, sizeof(int)*bytecodeCount);
	} else {
		int *x = 0;
		fprintf(stderr, "failed to allocate bytecode builder reached table\n");
		*x = 0;
	}

//	fprintf(stderr, "inlining generic method body for VMMethod %p %s\n", methodToInline, methodToInline->GetSignature()->GetChars());

	OMR::JitBuilder::BytecodeBuilder *merge = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	OMR::JitBuilder::BytecodeBuilder *previousBuilder = *ibuilder;

#if SOM_METHOD_DEBUG
	fprintf(stderr, "\n\t\tininling code for %s recLevel %d args%ld locals%ld\n", methodToInline->GetSignature()->GetChars(), recursiveLevel, methodToInline->GetNumberOfArguments(), methodToInline->GetNumberOfLocals());
#endif

	while (bcIndex < bytecodeCount) {
		OMR::JitBuilder::BytecodeBuilder *builder = bytecodeBuilderTable[bcIndex];
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
#if SOM_METHOD_DEBUG
		fprintf(stderr, "\t\t\tinline %ld %s", bcIndex, Bytecode::GetBytecodeName(bc));
#endif
		switch(bc) {
		case BC_DUP:
			DUP(builder);
			break;
		case BC_PUSH_LOCAL:
			pushValueFromArray(builder, getLocalArrayForLevel(builder, methodToInline, bcIndex, recursiveLevel), methodToInline->GetBytecode(bcIndex + 1));
			break;
		case BC_PUSH_FIELD:
			// TODO I am not sure receiver is the right object here.....but I can not call getSelf......
#if SOM_METHOD_DEBUG
			fprintf(stderr, " %d", methodToInline->GetBytecode(bcIndex + 1));
#endif
			pushField(builder, receiver, methodToInline->GetBytecode(bcIndex + 1));
//			builder->Call("printString", 1, builder->ConstInt64((int64_t)methodToInline->GetSignature()->GetChars()));
//			builder->Call("getClass", 1, receiver);
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
		case BC_PUSH_BLOCK:
		{
			VMMethod *blockMethod = static_cast<VMMethod*>(methodToInline->GetConstant(bcIndex));
			long numOfArgs = blockMethod->GetNumberOfArguments();

			blockMethods.push(blockMethod);

			OMR::JitBuilder::IlValue *block =
			builder->Call("getNewBlock", 3,
			builder->	ConstInt64((int64_t)recursiveLevel),
			builder->	ConstInt64((int64_t)blockMethod),
			builder->	ConstInt64((int64_t)numOfArgs));

			blockToReceiverMap.insert(std::make_pair((const void *)block, receiver));

			PUSH(builder, block);

			break;
		}
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
			OMR::JitBuilder::IlValue *sendReceiver = PICK(builder, numOfArgs - 1);

#if SOM_METHOD_DEBUG
			fprintf(stderr, " %s ", signature->GetChars());
#endif

			//Create a generic send location for the inlining.....
			//if something fails the JIT has to call the actual failing send with the current state....
			//can not fall back to the original sends generic send since state may have changed! and there is no way to roll it back

			OMR::JitBuilder::BytecodeBuilder *genericSend1 = nullptr;
			OMR::JitBuilder::BytecodeBuilder *mergeSend1 = nullptr;

			createBuildersForInlineSends(&genericSend1, &mergeSend1, bytecodeIndex);
			stackTopForErrorHandling[recursiveLevel + 1] = GET_STACKTOP(builder);
			int32_t extraDepth = stackTopForErrorHandling[recursiveLevel + 1] - stackTopForErrorHandling[0];
			if (extraDepth > extraStackDepthRequired) {
				extraStackDepthRequired = extraDepth;
			}
			inlinedMethods[recursiveLevel] = methodToInline;
			inlinedBytecodeIndecies[recursiveLevel] = bcIndex;

			SOMppMethod::RECOGNIZED_METHOD_INDEX recognizedMethodIndex = getRecognizedMethodIndex(methodToInline, receiverFromCache, invokableClass, signature->GetChars(), recursiveLevel + 1, false);
			if (NOT_RECOGNIZED != recognizedMethodIndex) {
				SOMppMethod::INLINE_STATUS status = generateRecognizedMethod(builder, &genericSend1, &mergeSend1, recognizedMethodIndex, receiverFromCache, bytecodeIndex, recursiveLevel + 1);
				if (status == INLINE_SUCCESSFUL_NO_GENERIC_PATH_REQUIRED) {
					genericSend1 = nullptr;
				}
			} else {
				generateGenericMethod(&builder, &genericSend1, &mergeSend1, receiverFromCache->LookupInvokable(signature), receiverFromCache, signature, bytecodeIndex, recursiveLevel + 1);
			}

			builder->Goto(mergeSend1);

			// need to update the method so the extra frame stack elements are allocated
//			fprintf(stderr, "send extraStack %d maxExtraStack %d\n", extraDepth, extraStackDepthRequired);

			//do generic send
			if (nullptr != genericSend1) {
				genericSend1->Store("receiverClass",
				genericSend1->	Call("getClass", 1, sendReceiver));

				/* going to call out of line helper so commit the stack */
				COMMIT(genericSend1);

//				genericSend1->Call("printString", 1, genericSend1->ConstInt64((int64_t)"failed "));
//				genericSend1->Call("printString", 1, genericSend1->ConstInt64((int64_t)methodToInline->GetSignature()->GetChars()));
//				genericSend1->Call("printString", 1, genericSend1->ConstInt64((int64_t)" of send "));
//				if (nullptr != receiverFromCache) {
//					genericSend1->Call("printString", 1, genericSend1->ConstInt64((int64_t)receiverFromCache->GetName()->GetChars()));
//					genericSend1->Call("printString", 1, genericSend1->ConstInt64((int64_t)">>:"));
//				}
//				genericSend1->Call("printString", 1, genericSend1->ConstInt64((int64_t)signature->GetChars()));
//				genericSend1->Call("printString", 1, genericSend1->ConstInt64((int64_t)"\n"));
//				genericSend1->Call("printObject", 3, sendReceiver, sendReceiver, sendReceiver);

				genericSend1->Store("previousFrameForSend",
				genericSend1->	Load("frame"));

				for (int32_t i = 0; i < recursiveLevel + 1; i++) {
					VMMethod *inlinedMethod = inlinedMethods[i];
					if (nullptr == inlinedMethod) {
						int *x = 0;
						fprintf(stderr, "Inlined method at %d is nullptr\n", i);
						*x = 0;
					}
					long inlinedNumOfArgs = inlinedMethod->GetNumberOfArguments();
					long inlinedNumOfLocals = inlinedMethod->GetNumberOfLocals();
					OMR::JitBuilder::IlValue *inlinedArgs = nullptr;
					OMR::JitBuilder::IlValue *inlinedLocals = nullptr;

					if (inlinedNumOfArgs > 0) {
						inlinedArgs = genericSend1->Load(getArgumentName(i));
					} else {
						inlinedArgs = genericSend1->ConstInt64(0);
					}

					if (inlinedNumOfLocals > 0) {
						inlinedLocals = genericSend1->Load(getLocalName(i));
					} else {
						inlinedLocals = genericSend1->ConstInt64(0);
					}

					genericSend1->Store("previousFrameForSend",
					genericSend1->	Call("allocateVMFrame", 8,
					genericSend1->		Load("interpreter"),
					genericSend1->		Load("previousFrameForSend"),
					genericSend1->		ConstInt64((int64_t)inlinedMethod),
										inlinedArgs,
										inlinedLocals,
					genericSend1->		LoadIndirect("VMFrame", "stack_ptr",
					genericSend1->			Load("frame")),
					genericSend1->		ConstInt64((int64_t)inlinedBytecodeIndecies[i]),
					genericSend1->		ConstInt64(i)));
				}

				genericSend1->Store("invokable",
				genericSend1->	Call("getInvokable", 2,
				genericSend1->		Load("receiverClass"),
				genericSend1->		ConstInt64((int64_t)signature)));

				genericSend1->StoreIndirect("VMFrame", "stack_ptr",
				genericSend1->	Load("previousFrameForSend"),
				genericSend1->	LoadIndirect("VMFrame", "stack_ptr",
				genericSend1->			Load("frame")));

				genericSend1->Store("return",
				genericSend1->	Call("doInlineSendIfRequired", 6,
				genericSend1->		Load("interpreter"),
				genericSend1->		Load("previousFrameForSend"),
				genericSend1->		Load("invokable"),
									sendReceiver,
				genericSend1->		ConstInt64((int64_t)signature),
				genericSend1->		ConstInt64((int64_t)bytecodeIndex)));

				//should not have to handle Block::restart or return of -1 since it would not be inlined.... maybe yes if I start handling all sends

				genericSend1->Store("sendResult",
				genericSend1->	LoadAt(pInt64,
				genericSend1->		LoadIndirect("VMFrame", "stack_ptr",
				genericSend1->	Load("previousFrameForSend"))));

				genericSend1->Goto(mergeSend1);
			}

			builder = mergeSend1;
			//end of generic send
			DROP(builder, numOfArgs);
			PUSH(builder, builder->Load("sendResult"));
			break;
		}
		case BC_JUMP_IF_FALSE:
		{
			OMR::JitBuilder::IlValue *value = POP(builder);
			OMR::JitBuilder::BytecodeBuilder *destBuilder = bytecodeBuilderTable[calculateBytecodeIndexForJump(methodToInline, bcIndex)];
			builder->IfCmpEqual(&destBuilder, value,
			builder->	ConstInt64((int64_t)falseObject));
			bytecodeTableEntryHasBeenReached[calculateBytecodeIndexForJump(methodToInline, bcIndex)] = 1;
			break;
		}
		case BC_JUMP_IF_TRUE:
		{
			OMR::JitBuilder::IlValue *value = POP(builder);
			OMR::JitBuilder::BytecodeBuilder *destBuilder = bytecodeBuilderTable[calculateBytecodeIndexForJump(methodToInline, bcIndex)];
			builder->IfCmpEqual(&destBuilder, value,
			builder->	ConstInt64((int64_t)trueObject));
			bytecodeTableEntryHasBeenReached[calculateBytecodeIndexForJump(methodToInline, bcIndex)] = 1;
			break;
		}
		case BC_JUMP:
		{
			OMR::JitBuilder::BytecodeBuilder *destBuilder = bytecodeBuilderTable[calculateBytecodeIndexForJump(methodToInline, bcIndex)];
			builder->Goto(&destBuilder);
			builder = NULL;
			bytecodeTableEntryHasBeenReached[calculateBytecodeIndexForJump(methodToInline, bcIndex)] = 1;
			break;
		}
		case BC_RETURN_LOCAL:
		{
			OMR::JitBuilder::IlValue *returnValue = POP(builder);
			builder->Store("sendResult", returnValue);
			builder->Goto(merge);
			builder = NULL;
			break;
		}
		case BC_RETURN_NON_LOCAL:
		{
			OMR::JitBuilder::IlValue *result = POP(builder);
			if (recursiveLevel == 0) {
				builder->Call("popFrameAndPushResult", 3,
				builder->	Load("interpreter"),
				builder->	Load("frame"), result);

				justReturn(builder);
			} else {
				builder->Store("sendResult", result);
//				fprintf(stderr, " recursive return non local \n");
				if (NULL == sendMerge) {
					fprintf(stderr, "About to crash on a NULL sendMerge\n");
				}
				generateInlineOSRToGenericSend(builder, sendMerge, bytecodeIndex, recursiveLevel);
			}
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
#if SOM_METHOD_DEBUG
		fprintf(stderr, "\n");
#endif
	}
	*ibuilder = merge;
	free(bytecodeTableEntryHasBeenReached);
	free(bytecodeBuilderTable);
}

void
SOMppMethod::generateGenericMethod(OMR::JitBuilder::BytecodeBuilder **b, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **merge, VMInvokable *invokable, VMClass *receiverClass, VMSymbol *signature, long bytecodeIndex, int32_t recursiveLevel)
{
	VMMethod *methodToInline = static_cast<VMMethod*>(invokable);
	long numOfArgs = methodToInline->GetNumberOfArguments();
	long numOfLocals = methodToInline->GetNumberOfLocals();
	OMR::JitBuilder::BytecodeBuilder *builder = *b;
	OMR::JitBuilder::IlValue *receiver = PICK(builder, numOfArgs - 1);
	const char *argumentName = getArgumentName(recursiveLevel);

	//if (recursiveLevel == 0) {
		//revist this as I am not sure it is always right.... do not set for blocks.... but other calls yes! but a block needs to use the right one if a new one is created in the block......
//		builder->Store(getSelfName(recursiveLevel), receiver);
	//}

//	fprintf(stderr, "inlining generic method for VMMethod %p %s %s\n", methodToInline, methodToInline->GetSignature()->GetChars(), receiverClass->GetName()->GetChars());

	verifyArg(builder, genericSend, receiver, builder->ConstInt64((int64_t)receiverClass), recursiveLevel);
//	verifyArg2(&builder, genericSend, receiver, builder->ConstInt64((int64_t)receiverClass), methodToInline, signature, bytecodeIndex, recursiveLevel);
	*b = builder;
	//verifyArg2 does not work with deltablue!!

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

	generateGenericMethodBody(b, genericSend, merge, methodToInline, receiver, bytecodeIndex, recursiveLevel);
}

void
SOMppMethod::createBuildersForInlineSends(OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **merge, long bytecodeIndex)
{
	if (nullptr == *genericSend) {
		*genericSend = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	}
	if (nullptr == *merge) {
		*merge = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	}
}

void
SOMppMethod::verifyIntegerArg(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::IlValue *integer, int32_t recursiveLevel)
{
#if USE_TAGGING
	OMR::JitBuilder::BytecodeBuilder *failurePath = nullptr;
	builder->IfCmpEqualZero(&failurePath,
	builder->	And(integer,
	builder->		ConstInt64(0x1)));

	SET_STACKTOP(failurePath, stackTopForErrorHandling[recursiveLevel]);
	failurePath->Goto(*genericSend);
#else
	verifyArg(builder, genericSend, integer, builder->ConstInt64((int64_t)integerClass), recursiveLevel);
#endif
}

void
SOMppMethod::verifyDoubleArg(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::IlValue *object, OMR::JitBuilder::IlValue *objectClass, int32_t recursiveLevel)
{
	builder->Store("isOk", builder->ConstInt32(1));
	OMR::JitBuilder::IlBuilder *notDouble = nullptr;
	builder->IfThen(&notDouble,
	builder->	NotEqualTo(objectClass, builder->ConstInt64((int64_t)doubleClass)));

	/*Inline verifyInteger */
#if USE_TAGGING
	OMR::JitBuilder::IlBuilder *notInteger = nullptr;
	notDouble->IfThen(&notInteger,
	notDouble->	NotEqualTo(
	notDouble->		And(object,
	notDouble->			ConstInt64(0x1)),
	notDouble->		ConstInt64(0x1)));

	notInteger->Store("isOk", notInteger->ConstInt32(0));
#else
	OMR::JitBuilder::IlBuilder *notInteger = nullptr;
	notDouble->IfThen(&notInteger,
	notDouble->	NotEqualTo(objectClass, notDouble->ConstInt64((int64_t)integerClass)));

	notInteger->Store("isOk", notInteger->ConstInt32(0));
#endif

	OMR::JitBuilder::BytecodeBuilder *failurePath = nullptr;
	builder->IfCmpNotEqual(&failurePath, builder->Load("isOk"), builder->ConstInt32(1));

	SET_STACKTOP(failurePath, stackTopForErrorHandling[recursiveLevel]);
	failurePath->Goto(*genericSend);
}

void
SOMppMethod::verifyArg(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::IlValue *object, OMR::JitBuilder::IlValue *type, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *objectClass =
	builder->Call("getClass", 1, object);

	OMR::JitBuilder::BytecodeBuilder *failurePath = nullptr;
	builder->IfCmpNotEqual(&failurePath, objectClass, type);

	SET_STACKTOP(failurePath, stackTopForErrorHandling[recursiveLevel]);
	failurePath->Goto(*genericSend);
}

void
SOMppMethod::verifyArg2(OMR::JitBuilder::BytecodeBuilder **b, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::IlValue *object, OMR::JitBuilder::IlValue *type, VMMethod *methodToInline, VMSymbol* signature, long bytecodeIndex, int32_t recursiveLevel)
{
	OMR::JitBuilder::BytecodeBuilder *builder = *b;

	OMR::JitBuilder::IlValue *objectClass =
	builder->Call("getClass", 1, object);

	OMR::JitBuilder::BytecodeBuilder *merge = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));

	OMR::JitBuilder::BytecodeBuilder *failurePath = nullptr;
	builder->IfCmpNotEqual(&failurePath, objectClass, type);

	builder->Goto(merge);
	*b = merge;

	OMR::JitBuilder::IlValue *invokable =
	failurePath->Call("getInvokable", 2, objectClass, failurePath->ConstInt64((int64_t)signature));

	OMR::JitBuilder::BytecodeBuilder *failurePath2 = nullptr;
	failurePath->IfCmpNotEqual(&failurePath2, invokable, failurePath->ConstInt64((int64_t)methodToInline));

//	failurePath->Call("printString", 1, failurePath->ConstInt64((int64_t)signature->GetChars()));
//
//	failurePath->Call("printObject", 3, object, type, object);

	failurePath->Goto(merge);

//	failurePath2->Call("printObject", 3, object, type, object);

	SET_STACKTOP(failurePath2, stackTopForErrorHandling[recursiveLevel]);
	failurePath2->Goto(*genericSend);
}

void
SOMppMethod::verifyBooleanArg(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::IlValue *object, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *superClass =
	builder->Call("getSuperClass", 1, object);

	OMR::JitBuilder::BytecodeBuilder *failurePath = nullptr;
	builder->IfCmpNotEqual(&failurePath, superClass, builder->ConstInt64((int64_t)booleanClass));

	failurePath->Call("printString", 1, failurePath->ConstInt64((int64_t)" not a boolean\n"));
	SET_STACKTOP(failurePath, stackTopForErrorHandling[recursiveLevel]);
	failurePath->Goto(*genericSend);
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForIntegerMath(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, MathFuncType mathFunction, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *receiver = PICK(builder, 1);
	OMR::JitBuilder::IlValue *param1 = TOP(builder);

	verifyIntegerArg(builder, genericSend, receiver, recursiveLevel);
	verifyIntegerArg(builder, genericSend, param1, recursiveLevel);

	OMR::JitBuilder::IlValue *receiverValue = getIntegerValue(builder, receiver);
	OMR::JitBuilder::IlValue *param1Value = getIntegerValue(builder, param1);
	OMR::JitBuilder::IlValue *newValue = (*mathFunction)(builder, receiverValue, param1Value);
	OMR::JitBuilder::IlValue *integerObject = newIntegerObjectForValue(builder, genericSend, newValue, recursiveLevel);

	builder->Store("sendResult", integerObject);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForIntegerBoolean(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, BooleanFuncType booleanFunction, OMR::JitBuilder::IlValue *thenPathValue, OMR::JitBuilder::IlValue *elsePathValue, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *receiver = PICK(builder, 1);
	OMR::JitBuilder::IlValue *param1 = TOP(builder);

	verifyIntegerArg(builder, genericSend, receiver, recursiveLevel);
	verifyIntegerArg(builder, genericSend, param1, recursiveLevel);

	OMR::JitBuilder::IlValue *receiverValue = getIntegerValue(builder, receiver);
	OMR::JitBuilder::IlValue *param1Value = getIntegerValue(builder, param1);

	OMR::JitBuilder::IlBuilder *thenPath = nullptr;
	OMR::JitBuilder::IlBuilder *elsePath = nullptr;
	builder->IfThenElse(&thenPath, &elsePath, (*booleanFunction)(builder, receiverValue, param1Value));

	thenPath->Store("sendResult", thenPathValue);

	elsePath->Store("sendResult", elsePathValue);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForIntegerNegated(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *receiver = TOP(builder);

	verifyIntegerArg(builder, genericSend, receiver, recursiveLevel);

	OMR::JitBuilder::IlValue *receiverValue = getIntegerValue(builder, receiver);

	OMR::JitBuilder::IlValue *newValue =
	builder->Sub(
	builder->	ConstInt64(0), receiverValue);

	OMR::JitBuilder::IlValue *integerObject = newIntegerObjectForValue(builder, genericSend, newValue, recursiveLevel);

	builder->Store("sendResult", integerObject);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForIntegerMax(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *receiver = PICK(builder, 1);
	OMR::JitBuilder::IlValue *param1 = TOP(builder);

	verifyIntegerArg(builder, genericSend, receiver, recursiveLevel);
	verifyIntegerArg(builder, genericSend, param1, recursiveLevel);

	OMR::JitBuilder::IlValue *receiverValue = getIntegerValue(builder, receiver);
	OMR::JitBuilder::IlValue *param1Value = getIntegerValue(builder, param1);

	OMR::JitBuilder::IlBuilder *thenPath = nullptr;
	OMR::JitBuilder::IlBuilder *elsePath = nullptr;
	builder->IfThenElse(&thenPath, &elsePath,
	builder->	GreaterThan(receiverValue, param1Value));

	thenPath->Store("sendResult", receiver);

	elsePath->Store("sendResult", param1);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForIntegerAbs(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, long bytecodeIndex, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *receiver = TOP(builder);

	verifyIntegerArg(builder, genericSend, receiver, recursiveLevel);

	builder->Store("absValue", getIntegerValue(builder, receiver));

	OMR::JitBuilder::IlBuilder *thenPath = nullptr;
	builder->IfThen(&thenPath,
	builder->	LessThan(
	builder->		Load("absValue"),
	builder->		ConstInt64(0)));

	thenPath->Store("absValue",
	thenPath->	Sub(
	thenPath->		ConstInt64(0),
	thenPath->		Load("absValue")));

	builder->Store("sendResult", newIntegerObjectForValue(builder, genericSend, builder->Load("absValue"), recursiveLevel));

	return INLINE_SUCCESSFUL;
}

static const char * getIndexName(int64_t recursiveLevel)
{
	if (recursiveLevel == 0) {
		return "_i0";
	} else if (recursiveLevel == 1) {
		return "_i1";
	} else if (recursiveLevel == 2) {
		return "_i2";
	} else if (recursiveLevel == 3) {
		return "_i3";
	} else if (recursiveLevel == 4) {
		return "_i4";
	} else {
		return nullptr;
	}
}

void
SOMppMethod::generateInlineForIntegerLoop(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, ForLoopFuncType loopFunction, OMR::JitBuilder::IlValue *start, OMR::JitBuilder::IlValue *end, OMR::JitBuilder::IlValue *increment, OMR::JitBuilder::IlValue *block, VMMethod *blockToInline, int32_t recursiveLevel)
{
	const char *arguements = getArgumentName(recursiveLevel);
	const char *localsName = getLocalName(recursiveLevel);

	blockToInline = blockMethods.top();
	blockMethods.pop();

	builder->Store(arguements,
	builder->	CreateLocalArray((int32_t)2, Int64));

	long numOfLocals = blockToInline->GetNumberOfLocals();
	if (numOfLocals == 0) {
		numOfLocals = 1;//need at least one local in case this is a loop than contains another for loop
	}
//	fprintf(stderr, "SOMppMethod::generateInlineForIntegerLoop numArgs %d numLocals %d recursiveLevel %d\n", 2, numOfLocals, recursiveLevel);
	if (numOfLocals > 0) {
		builder->Store(localsName,
		builder->	CreateLocalArray((int32_t)numOfLocals, Int64));

		for (long i = 0; i < numOfLocals; i++) {
			builder->StoreAt(
			builder->	IndexAt(pInt64,
			builder->		ConvertTo(pInt64,
			builder->			Load(localsName)),
			builder->		ConstInt32((int32_t)i)),
			builder->	ConstInt64((int64_t)nilObject));
		}
	}

	const char *indexName = getIndexName(recursiveLevel);

	builder->StoreAt(
	builder->	IndexAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			Load(arguements)),
	builder->		ConstInt32(0)), block);

	builder->StoreAt(
	builder-> IndexAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->         Load(arguements)),
	builder->         ConstInt32(1)), start);

	OMR::JitBuilder::IlValue *self = nullptr;
//	if (0 == recursiveLevel) {
//		self = builder->Load(getSelfName(recursiveLevel));//getSelf(builder);
//	} else {
//		self = builder->Load(getSelfName(recursiveLevel - 1));//getSelf(builder);
//	}

	auto search = blockToReceiverMap.find((const void *)block);
	if(search != blockToReceiverMap.end()) {
		self = search->second;
	} else {
		int *x = 0;
		fprintf(stderr, "Did not find receiver for block %p\n", block);
		*x = 0;
	}

	OMR::JitBuilder::BytecodeBuilder *iloop = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	builder->AddSuccessorBuilder(&iloop);

	OMR::JitBuilder::BytecodeBuilder *first = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	OMR::JitBuilder::BytecodeBuilder *last = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));

	iloop->AddSuccessorBuilder(&first);
	iloop->AppendBuilder(first);
	iloop->AddSuccessorBuilder(&last);
	iloop->AppendBuilder(last);

	// Initialize the loop after the loop code has been created!
	OMR::JitBuilder::IlBuilder *looper = (OMR::JitBuilder::IlBuilder *)iloop;
	loopFunction(builder, indexName, &looper, start, end, increment);

	/* START OF LOOP */
	OMR::JitBuilder::IlValue *i = first->Load(indexName);
	OMR::JitBuilder::IlValue *iValue = newIntegerObjectForValue(first, genericSend, i, recursiveLevel);

	first->StoreAt(
	first-> IndexAt(pInt64,
	first->		ConvertTo(pInt64,
	first->         Load(arguements)),
	first->         ConstInt32(1)), iValue);

#if SOM_METHOD_DEBUG
	fprintf(stderr, " %s>>#%s ", blockToInline->GetHolder()->GetName()->GetChars(), blockToInline->GetSignature()->GetChars());
#endif

	generateGenericMethodBody(&first, genericSend, mergeSend, blockToInline, self, bytecodeIndex, recursiveLevel);

	first->Goto(last);
	/* END OF LOOP */

//	OMR::JitBuilder::IlValue *locals = nullptr;
//	if (recursiveLevel == 0) {
//		locals = builder->Load("frameLocals");
//	} else {
//		locals = builder->Load(getLocalName(recursiveLevel - 1));
//	}
//	OMR::JitBuilder::IlValue *value =
//	builder->LoadAt(pInt64,
//	builder->	IndexAt(pInt64,
//	builder->		ConvertTo(pInt64, locals),
//	builder->		ConstInt64(0)));

//	builder->Store("sendResult", value);
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForIntegerToByDo(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *loopStart = PICK(builder, 3);
	OMR::JitBuilder::IlValue *loopEnd = PICK(builder, 2);
	OMR::JitBuilder::IlValue *loopIncrement = PICK(builder, 1);
	OMR::JitBuilder::IlValue *block = PICK(builder, 0);

	verifyIntegerArg(builder, genericSend, loopStart, recursiveLevel);
	verifyIntegerArg(builder, genericSend, loopEnd, recursiveLevel);
	verifyIntegerArg(builder, genericSend, loopIncrement, recursiveLevel);

	OMR::JitBuilder::IlValue *loopStartValue = getIntegerValue(builder, loopStart);
	OMR::JitBuilder::IlValue *loopEndValue = getIntegerValue(builder, loopEnd);
	/* need to add 1 to the loop end */
	OMR::JitBuilder::IlValue *actualLoopEnd = builder->Add(loopEndValue, builder->ConstInt64(1));
	OMR::JitBuilder::IlValue *loopIncrementValue = getIntegerValue(builder, loopIncrement);

	generateInlineForIntegerLoop(builder, genericSend, mergeSend, bytecodeIndex, &SOMppMethod::forLoopUp, loopStartValue, actualLoopEnd, loopIncrementValue, block, forLoopBlock, recursiveLevel);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForIntegerToDo(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *loopStart = PICK(builder, 2); /* receiver for the Integer>>#to:do: (initial loop) */
	OMR::JitBuilder::IlValue *loopEnd = PICK(builder, 1); /* the integer for to: (loop end) */
	OMR::JitBuilder::IlValue *block = PICK(builder, 0); /* the block */

	verifyIntegerArg(builder, genericSend, loopStart, recursiveLevel);
	verifyIntegerArg(builder, genericSend, loopEnd, recursiveLevel);

	OMR::JitBuilder::IlValue *loopStartValue = getIntegerValue(builder, loopStart);
	OMR::JitBuilder::IlValue *loopEndValue = getIntegerValue(builder, loopEnd);
	/* need to add 1 to the loop end */
	OMR::JitBuilder::IlValue *actualLoopEnd = builder->Add(loopEndValue, builder->ConstInt64(1));
	OMR::JitBuilder::IlValue *loopIncrementValue = builder->ConstInt64(1);

	generateInlineForIntegerLoop(builder, genericSend, mergeSend, bytecodeIndex, &SOMppMethod::forLoopUp, loopStartValue, actualLoopEnd, loopIncrementValue, block, forLoopBlock, recursiveLevel);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForIntegerDownToDo(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *loopStart = PICK(builder, 2); /* receiver for the Integer>>#to:do: (initial loop) */
	OMR::JitBuilder::IlValue *loopEnd = PICK(builder, 1); /* the integer for to: (loop end) */
	OMR::JitBuilder::IlValue *block = PICK(builder, 0); /* the block */

	verifyIntegerArg(builder, genericSend, loopStart, recursiveLevel);
	verifyIntegerArg(builder, genericSend, loopEnd, recursiveLevel);

	OMR::JitBuilder::IlValue *loopStartValue = getIntegerValue(builder, loopStart);
	OMR::JitBuilder::IlValue *loopEndValue = getIntegerValue(builder, loopEnd);
	OMR::JitBuilder::IlValue *loopIncrementValue = builder->ConstInt64(1);

	generateInlineForIntegerLoop(builder, genericSend, mergeSend, bytecodeIndex, &SOMppMethod::forLoopDown, loopStartValue, loopEndValue, loopIncrementValue, block, forLoopBlock, recursiveLevel);

	return INLINE_SUCCESSFUL;
}
SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForIntegerDownToByDo(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *loopStart = PICK(builder, 3);
	OMR::JitBuilder::IlValue *loopEnd = PICK(builder, 2);
	OMR::JitBuilder::IlValue *loopIncrement = PICK(builder, 1);
	OMR::JitBuilder::IlValue *block = PICK(builder, 0);

	verifyIntegerArg(builder, genericSend, loopStart, recursiveLevel);
	verifyIntegerArg(builder, genericSend, loopEnd, recursiveLevel);
	verifyIntegerArg(builder, genericSend, loopIncrement, recursiveLevel);

	OMR::JitBuilder::IlValue *loopStartValue = getIntegerValue(builder, loopStart);
	OMR::JitBuilder::IlValue *loopEndValue = getIntegerValue(builder, loopEnd);
	OMR::JitBuilder::IlValue *loopIncrementValue = getIntegerValue(builder, loopIncrement);

	generateInlineForIntegerLoop(builder, genericSend, mergeSend, bytecodeIndex, &SOMppMethod::forLoopDown, loopStartValue, loopEndValue, loopIncrementValue, block, forLoopBlock, recursiveLevel);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForArrayAt(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *array = PICK(builder, 1);
	OMR::JitBuilder::IlValue *index = PICK(builder, 0);

	verifyArg(builder, genericSend, array, builder->ConstInt64((int64_t)arrayClass), recursiveLevel);
	verifyIntegerArg(builder, genericSend, index, recursiveLevel);

	builder->Store("sendResult",
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64, getIndexableFieldSlot(builder, array, index))));

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForArrayAtPut(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *array = PICK(builder, 2);
	OMR::JitBuilder::IlValue *index = PICK(builder, 1);
	OMR::JitBuilder::IlValue *object = PICK(builder, 0);

	verifyArg(builder, genericSend, array, builder->ConstInt64((int64_t)arrayClass), recursiveLevel);
	verifyIntegerArg(builder, genericSend, index, recursiveLevel);

	builder->StoreAt(
	builder->	ConvertTo(pInt64, getIndexableFieldSlot(builder, array, index)), object);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForArrayLength(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *array = PICK(builder, 0);

	verifyArg(builder, genericSend, array, builder->ConstInt64((int64_t)arrayClass), recursiveLevel);

//	OMR::JitBuilder::BytecodeBuilder *temp = *genericSend;
//	temp->Call("printString", 1, temp->ConstInt64((int64_t)"Damn it to hell\n"));

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

	OMR::JitBuilder::IlValue *integerObject = newIntegerObjectForValue(builder, genericSend, numberOfIndexableFields, recursiveLevel);

	builder->Store("sendResult", integerObject);

	return INLINE_SUCCESSFUL;
}


static OMR::JitBuilder::IlValue *
getIndexableFieldSlotFromValue(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *array, OMR::JitBuilder::IlValue *indexValue)
{
	OMR::JitBuilder::IlValue *numberOfFields = builder->LoadIndirect("VMObject", "numberOfFields", array);
	OMR::JitBuilder::IlValue *vmObjectSize = builder->ConstInt64(sizeof(VMObject));
	OMR::JitBuilder::IlValue *vmObjectPointerSize = builder->ConstInt64(sizeof(VMObject*));
	OMR::JitBuilder::IlValue *indexableIndex = builder->Add(indexValue, numberOfFields);

	OMR::JitBuilder::IlValue *actualIndex =
	builder->Sub(indexableIndex,
	builder->	ConstInt64(1));

	OMR::JitBuilder::IlValue *offset =
	builder->Add(vmObjectSize,
	builder->	Mul(actualIndex, vmObjectPointerSize));

	return builder->Add(array, offset);
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForArrayDo(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *array = PICK(builder, 1);

	//START OF ARRAY LENGTH HELPER
	verifyArg(builder, genericSend, array, builder->ConstInt64((int64_t)arrayClass), recursiveLevel);

//	OMR::JitBuilder::BytecodeBuilder *temp = *genericSend;
//	temp->Call("printString", 1, temp->ConstInt64((int64_t)"Damn it to hell\n"));

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

	//END OF ARRAY LENGTH HELPER

	OMR::JitBuilder::IlValue *block = PICK(builder, 0);

	OMR::JitBuilder::IlValue *one = builder->ConstInt64(1);
	OMR::JitBuilder::IlValue *actualEnd = builder->Add(numberOfIndexableFields, one);//Add 1 to the end for the loop

	//START OF LOOP HELPER
	const char *arguements = getArgumentName(recursiveLevel);
	const char *localsName = getLocalName(recursiveLevel);

	VMMethod *blockToInline = blockMethods.top();
	blockMethods.pop();

	builder->Store(arguements,
	builder->	CreateLocalArray((int32_t)2, Int64));

	long numOfLocals = blockToInline->GetNumberOfLocals();
	if (numOfLocals == 0) {
		numOfLocals = 1;//need at least one local in case this is a loop than contains another for loop
	}

	if (numOfLocals > 0) {
		builder->Store(localsName,
		builder->	CreateLocalArray((int32_t)numOfLocals, Int64));

		for (long i = 0; i < numOfLocals; i++) {
			builder->StoreAt(
			builder->	IndexAt(pInt64,
			builder->		ConvertTo(pInt64,
			builder->			Load(localsName)),
			builder->		ConstInt32((int32_t)i)),
			builder->	ConstInt64((int64_t)nilObject));
		}
	}

	const char *indexName = getIndexName(recursiveLevel);

	builder->StoreAt(
	builder->	IndexAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			Load(arguements)),
	builder->		ConstInt32(0)), block);

	builder->StoreAt(
	builder-> IndexAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			Load(arguements)),
	builder->		ConstInt32(1)),
	builder->	LoadAt(pInt64,
	builder->		ConvertTo(pInt64, getIndexableFieldSlotFromValue(builder, array, one))));

//	OMR::JitBuilder::IlValue *self = getSelf(builder);
	OMR::JitBuilder::IlValue *self = nullptr;
	auto search = blockToReceiverMap.find((const void *)block);
	if(search != blockToReceiverMap.end()) {
		self = search->second;
	} else {
		int *x = 0;
		fprintf(stderr, "Did not find receiver for arrayDo: block %p\n", block);
		*x = 0;
	}

	OMR::JitBuilder::BytecodeBuilder *iloop = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	builder->AddSuccessorBuilder(&iloop);

	OMR::JitBuilder::BytecodeBuilder *first = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	OMR::JitBuilder::BytecodeBuilder *last = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));

	iloop->AddSuccessorBuilder(&first);
	iloop->AppendBuilder(first);
	iloop->AddSuccessorBuilder(&last);
	iloop->AppendBuilder(last);

	// Initialize the loop after the loop code has been created!
	OMR::JitBuilder::IlBuilder *looper = (OMR::JitBuilder::IlBuilder *)iloop;
	SOMppMethod::forLoopUp(builder, indexName, &looper, one, actualEnd, one);

	/* START OF LOOP */
	OMR::JitBuilder::IlValue *i = first->Load(indexName);

	first->StoreAt(
	first-> IndexAt(pInt64,
	first->		ConvertTo(pInt64,
	first->			Load(arguements)),
	first->		ConstInt32(1)),
	first->	LoadAt(pInt64,
	first->		ConvertTo(pInt64, getIndexableFieldSlotFromValue(first, array,i))));

//	first->Call("printString", 1, first->ConstInt64((int64_t)""));
//	first->Call("printInt64", 1, getIntegerValue(first, first->LoadAt(pInt64,first->ConvertTo(pInt64, getIndexableFieldSlotFromValue(first, array, i)))));
//	first->Call("printString", 1, first->ConstInt64((int64_t)" "));
//	first->Call("printInt64", 1, i);
//	first->Call("printString", 1, first->ConstInt64((int64_t)"\n"));

#if SOM_METHOD_DEBUG
	fprintf(stderr, " %s>>#%s ", blockToInline->GetHolder()->GetName()->GetChars(), blockToInline->GetSignature()->GetChars());
#endif

	generateGenericMethodBody(&first, genericSend, mergeSend, blockToInline, self, bytecodeIndex, recursiveLevel);

	first->Goto(last);
	/* END OF LOOP */

	OMR::JitBuilder::IlValue *locals = nullptr;
	if (recursiveLevel == 0) {
		locals = builder->Load("frameLocals");
	} else {
		locals = builder->Load(getLocalName(recursiveLevel - 1));
	}
	OMR::JitBuilder::IlValue *value =
	builder->LoadAt(pInt64,
	builder->	IndexAt(pInt64,
	builder->		ConvertTo(pInt64, locals),
	builder->		ConstInt64(0)));

	builder->Store("sendResult", value);

	//END OF LOOP HELPER

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForArrayDoIndexes(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *array = PICK(builder, 1);

	//START OF ARRAY LENGTH HELPER
	verifyArg(builder, genericSend, array, builder->ConstInt64((int64_t)arrayClass), recursiveLevel);

//	OMR::JitBuilder::BytecodeBuilder *temp = *genericSend;
//	temp->Call("printString", 1, temp->ConstInt64((int64_t)"Damn it to hell\n"));

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

	//END OF ARRAY LENGTH HELPER

	OMR::JitBuilder::IlValue *block = PICK(builder, 0);

	OMR::JitBuilder::IlValue *one = builder->ConstInt64(1);

	OMR::JitBuilder::IlValue *actualEnd = builder->Add(numberOfIndexableFields, one);//Add 1 to the end for the loop

	generateInlineForIntegerLoop(builder, genericSend, mergeSend, bytecodeIndex, &SOMppMethod::forLoopUp, one, actualEnd, one, block, forLoopBlock, recursiveLevel);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForArrayNew(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *length = PICK(builder, 0);

	verifyIntegerArg(builder, genericSend, length, recursiveLevel);
	OMR::JitBuilder::BytecodeBuilder *temp = *genericSend;
	temp->Call("printString", 1, temp->ConstInt64((int64_t)"Damn it to hell\n"));

	builder->Store("sendResult",
	builder->	Call("newArray", 1, getIntegerValue(builder, length)));

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForDoubleMath(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, MathFuncType mathFunction, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *receiver = PICK(builder, 1);
	OMR::JitBuilder::IlValue *param1 = TOP(builder);

	verifyArg(builder, genericSend, receiver, builder->ConstInt64((int64_t)doubleClass), recursiveLevel);

	OMR::JitBuilder::IlValue *param1Class = builder->Call("getClass", 1, param1);
	verifyDoubleArg(builder, genericSend, param1, param1Class, recursiveLevel);
//	verifyArg(builder, genericSend, param1, builder->ConstInt64((int64_t)doubleClass), recursiveLevel);

	OMR::JitBuilder::IlValue *receiverValue = getDoubleValue(builder, receiver);
	OMR::JitBuilder::IlValue *param1Value = getDoubleValueFromDoubleOrInteger(builder, param1, param1Class);
//	OMR::JitBuilder::IlValue *param1Value = getDoubleValue(builder, param1);
	OMR::JitBuilder::IlValue *newValue = (*mathFunction)(builder, receiverValue, param1Value);

	builder->Store("sendResult",
	builder->	Call("newDouble", 1, newValue));

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForDoubleBoolean(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, BooleanFuncType booleanFunction, OMR::JitBuilder::IlValue *thenPathValue, OMR::JitBuilder::IlValue *elsePathValue, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *receiver = PICK(builder, 1);
	OMR::JitBuilder::IlValue *param1 = TOP(builder);

	verifyArg(builder, genericSend, receiver, builder->ConstInt64((int64_t)doubleClass), recursiveLevel);

//	OMR::JitBuilder::IlValue *param1Class = builder->Call("getClass", 1, param1);
//	verifyDoubleArg(builder, genericSend, param1, param1Class, recursiveLevel);
	verifyArg(builder, genericSend, param1, builder->ConstInt64((int64_t)doubleClass), recursiveLevel);

	OMR::JitBuilder::IlValue *receiverValue = getDoubleValue(builder, receiver);
//	OMR::JitBuilder::IlValue *param1Value = getDoubleValueFromDoubleOrInteger(builder, param1, param1Class);
	OMR::JitBuilder::IlValue *param1Value = getDoubleValue(builder, param1);

	OMR::JitBuilder::IlBuilder *thenPath = nullptr;
	OMR::JitBuilder::IlBuilder *elsePath = nullptr;
	builder->IfThenElse(&thenPath, &elsePath, (*booleanFunction)(builder, receiverValue, param1Value));

	thenPath->Store("sendResult", thenPathValue);

	elsePath->Store("sendResult", elsePathValue);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForObjectNotEqual(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, VMClass *receiverClass, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *receiver = PICK(builder, 1);
	OMR::JitBuilder::IlValue *param1 = PICK(builder, 0);

	verifyArg(builder, genericSend, receiver, builder->ConstInt64((int64_t)receiverClass), recursiveLevel);

	OMR::JitBuilder::IlBuilder *thenPath = nullptr;
	OMR::JitBuilder::IlBuilder *elsePath = nullptr;
	builder->IfThenElse(&thenPath, &elsePath,
	builder->	NotEqualTo(receiver, param1));

	thenPath->Store("sendResult",
	thenPath->	ConstInt64((int64_t)trueObject));

	elsePath->Store("sendResult",
	elsePath->	ConstInt64((int64_t)falseObject));

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForObjectEqual(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, VMClass *receiverClass, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *receiver = PICK(builder, 1);
	OMR::JitBuilder::IlValue *param1 = PICK(builder, 0);

	verifyArg(builder, genericSend, receiver, builder->ConstInt64((int64_t)receiverClass), recursiveLevel);

	OMR::JitBuilder::IlBuilder *thenPath = nullptr;
	OMR::JitBuilder::IlBuilder *elsePath = nullptr;
	builder->IfThenElse(&thenPath, &elsePath,
	builder->	EqualTo(receiver, param1));

	thenPath->Store("sendResult",
	thenPath->	ConstInt64((int64_t)trueObject));

	elsePath->Store("sendResult",
	elsePath->	ConstInt64((int64_t)falseObject));

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForObjectValue(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, VMClass *receiverClass, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *receiver = PICK(builder, 0);

	verifyArg(builder, genericSend, receiver, builder->ConstInt64((int64_t)receiverClass), recursiveLevel);

	builder->Store("sendResult", receiver);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForGenericIsNil(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlValue *receiver = PICK(builder, 0);

	OMR::JitBuilder::IlBuilder *thenPath = nullptr;
	OMR::JitBuilder::IlBuilder *elsePath = nullptr;
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
SOMppMethod::generateInlineForGenericNotNil(OMR::JitBuilder::BytecodeBuilder *builder)
{
	OMR::JitBuilder::IlValue *receiver = PICK(builder, 0);

	OMR::JitBuilder::IlBuilder *thenPath = nullptr;
	OMR::JitBuilder::IlBuilder *elsePath = nullptr;
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

static const char * getConditionName(int64_t recursiveLevel)
{
	if (recursiveLevel == 0) {
		return "_keepIterating0";
	} else if (recursiveLevel == 1) {
		return "_keepIterating1";
	} else if (recursiveLevel == 2) {
		return "_keepIterating2";
	} else if (recursiveLevel == 3) {
		return "_keepIterating3";
	} else if (recursiveLevel == 4) {
		return "_keepIterating4";
	} else {
		return nullptr;
	}
}

void
SOMppMethod::generateForWhileLoop(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel, OMR::JitBuilder::IlValue *condition)
{
	VMMethod *codeBlock = blockMethods.top();
	blockMethods.pop();
	VMMethod *conditionBlock = blockMethods.top();
	blockMethods.pop();

	const char *condtionName = getConditionName(recursiveLevel);

//	OMR::JitBuilder::IlValue *self = getSelf(builder);
	OMR::JitBuilder::IlValue *self = nullptr;
	auto search = blockToReceiverMap.find((const void *)PICK(builder, 0));
	if(search != blockToReceiverMap.end()) {
		self = search->second;
	} else {
		int *x = 0;
		fprintf(stderr, "Did not find receiver for block %p\n", PICK(builder, 0));
		*x = 0;
	}

	const char *arguements = getArgumentName(recursiveLevel);
	builder->Store(arguements,
	builder->	CreateLocalArray((int32_t)1, Int64));

	builder->StoreAt(
	builder->	IndexAt(pInt64,
	builder->		ConvertTo(pInt64,
	builder->			Load(arguements)),
	builder->		ConstInt32((int32_t)0)),
	builder->	ConstInt64((int64_t)nilObject));

	long numOfLocals = codeBlock->GetNumberOfLocals();
	if (numOfLocals == 0) {
		numOfLocals = 1; //We may inline an integer loop inside this loop
	}
	if (numOfLocals > 0) {
		const char* locals = getLocalName(recursiveLevel);
		builder->Store(locals,
		builder->	CreateLocalArray((int32_t)numOfLocals, Int64));

		for (long i = 0; i < numOfLocals; i++) {
			builder->StoreAt(
			builder->	IndexAt(pInt64,
			builder->		ConvertTo(pInt64,
			builder->			Load(locals)),
			builder->		ConstInt32((int32_t)i)),
			builder->	ConstInt64((int64_t)nilObject));
		}
	}

	OMR::JitBuilder::BytecodeBuilder *preStart = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	OMR::JitBuilder::BytecodeBuilder *preEnd = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));

	builder->AddSuccessorBuilder(&preStart);
	builder->AppendBuilder(preStart);
	builder->AddSuccessorBuilder(&preEnd);
	builder->AppendBuilder(preEnd);

	/* generate the initial loop condition. */
	generateGenericMethodBody(&preStart, genericSend, mergeSend, conditionBlock, self, bytecodeIndex, recursiveLevel);

	preStart->Goto(preEnd);

	preEnd->Store(condtionName,
	preEnd->	EqualTo(
	preEnd->		Load("sendResult"), condition));

	OMR::JitBuilder::BytecodeBuilder *loopBody = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	preEnd->AddSuccessorBuilder(&loopBody);
	OMR::JitBuilder::IlBuilder *looper = (OMR::JitBuilder::IlBuilder *)loopBody;

	OMR::JitBuilder::BytecodeBuilder *first = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	OMR::JitBuilder::BytecodeBuilder *last = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));

	loopBody->AddSuccessorBuilder(&first);
	loopBody->AppendBuilder(first);
	loopBody->AddSuccessorBuilder(&last);
	loopBody->AppendBuilder(last);

	preEnd->WhileDoLoop(const_cast<char*>(condtionName), &looper);

	/* START OF LOOP */
#if SOM_METHOD_DEBUG
	fprintf(stderr, " %s>>#%s locals%ld", codeBlock->GetHolder()->GetName()->GetChars(), codeBlock->GetSignature()->GetChars(), numOfLocals);
#endif
	generateGenericMethodBody(&first, genericSend, mergeSend, codeBlock, self, bytecodeIndex, recursiveLevel);

	/* Generate code for the loop condition */
	generateGenericMethodBody(&first, genericSend, mergeSend, conditionBlock, self, bytecodeIndex, recursiveLevel);
	first->Goto(last);

	last->Store(condtionName,
	last->	EqualTo(
	last->		Load("sendResult"), condition));
	/* END OF LOOP */

	preEnd->Store("sendResult",
	preEnd->	ConstInt64((int64_t)nilObject));
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForWhileTrue(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel)
{
	//OMR::JitBuilder::IlValue *block2 = PICK(builder, 1);
	OMR::JitBuilder::IlValue *block1 = PICK(builder, 0);

	//TODO call real verify
	OMR::JitBuilder::IlValue *objectClass =
	builder->Call("getSuperClass", 1, block1);

	OMR::JitBuilder::BytecodeBuilder *failurePath = nullptr;
	builder->IfCmpNotEqual(&failurePath, objectClass, builder->ConstInt64((int64_t)blockClass));

	SET_STACKTOP(failurePath, stackTopForErrorHandling[recursiveLevel]);
	failurePath->Goto(*genericSend);

	//TODO call verify for block2

	generateForWhileLoop(builder, genericSend, mergeSend, bytecodeIndex, recursiveLevel, builder->ConstInt64((int64_t)trueObject));
	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForWhileFalse(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel)
{
	//OMR::JitBuilder::IlValue *block2 = PICK(builder, 1);
	OMR::JitBuilder::IlValue *block1 = PICK(builder, 0);

	//TODO call real verify
	OMR::JitBuilder::IlValue *objectClass =
	builder->Call("getSuperClass", 1, block1);

	OMR::JitBuilder::BytecodeBuilder *failurePath = nullptr;
	builder->IfCmpNotEqual(&failurePath, objectClass, builder->ConstInt64((int64_t)blockClass));

	SET_STACKTOP(failurePath, stackTopForErrorHandling[recursiveLevel]);
	failurePath->Goto(*genericSend);

	//TODO call verify for block2

	generateForWhileLoop(builder, genericSend, mergeSend, bytecodeIndex, recursiveLevel, builder->ConstInt64((int64_t)falseObject));
	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForBooleanAnd(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *receiver = PICK(builder, 1);
	OMR::JitBuilder::IlValue *param1 = PICK(builder, 0);

	VMMethod *codeBlock = blockMethods.top();
	blockMethods.pop();

	const char *arguements = getArgumentName(recursiveLevel);
//	const char *localsName = getLocalName(recursiveLevel);

	long numOfArgs = codeBlock->GetNumberOfArguments();

	builder->Store(arguements,
	builder->	CreateLocalArray((int32_t)numOfArgs, Int64));

	for (int32_t i = 0; i < (int32_t)numOfArgs; i++) {
		builder->StoreAt(
		builder->	IndexAt(pInt64,
		builder->		ConvertTo(pInt64,
		builder->			Load(arguements)),
		builder->		ConstInt32(i)),
					PICK(builder, numOfArgs - 1 - i));
	}

//	OMR::JitBuilder::IlValue *self = getSelf(builder);
//	OMR::JitBuilder::IlValue *self2 = builder->Load(getSelfName(recursiveLevel));///todo come back as this did not fix the new: inlining issue
	OMR::JitBuilder::IlValue *self2 = nullptr;
	auto search = blockToReceiverMap.find((const void *)param1);
	if(search != blockToReceiverMap.end()) {
		self2 = search->second;
	} else {
		int *x = 0;
		fprintf(stderr, "Did not find receiver for block %p\n", param1);
		*x = 0;
	}

	//TODO do I need locals?

	//IF receiver == falseObject then return false
	OMR::JitBuilder::BytecodeBuilder *falsePath = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	OMR::JitBuilder::BytecodeBuilder *truePath = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	builder->AddSuccessorBuilder(&falsePath);
	builder->AddSuccessorBuilder(&truePath);

	OMR::JitBuilder::IlBuilder *fPath = (OMR::JitBuilder::IlBuilder *)falsePath;
	OMR::JitBuilder::IlBuilder *tPath = (OMR::JitBuilder::IlBuilder *)truePath;
	builder->IfThenElse(&fPath, &tPath,
	builder->	EqualTo(
	builder->		ConstInt64((int64_t)falseObject), receiver));

//	OMR::JitBuilder::IlValue *objectClass =
//	falsePath->Call("getClass", 1, receiver);

	OMR::JitBuilder::BytecodeBuilder *failurePath = nullptr;
//	falsePath->IfCmpNotEqual(&failurePath, objectClass, falsePath->ConstInt64((int64_t)booleanClass));
//
//	//failurePath->Call("printString", 1, failurePath->ConstInt64((int64_t)"boolean and and param is not a block\n"));
	SET_STACKTOP(failurePath, stackTopForErrorHandling[recursiveLevel]);
//	failurePath->Goto(*genericSend);

	falsePath->Store("sendResult", receiver);

	//should verify the receiver is a boolean

//	fprintf(stderr, "generateInlineForBooleanAnd\n");

	//VERIFY That the parameter is a block
	OMR::JitBuilder::IlValue *objectClass2 =
	truePath->Call("getSuperClass", 1, param1);

	truePath->IfCmpNotEqual(&failurePath, objectClass2, truePath->ConstInt64((int64_t)blockClass));

	//failurePath->Call("printString", 1, failurePath->ConstInt64((int64_t)"boolean and and param is not a block\n"));
	SET_STACKTOP(failurePath, stackTopForErrorHandling[recursiveLevel]);
	failurePath->Goto(*genericSend);

	//TODO switch to proper verification that it is a block class



	/* START OF BLOCK */

	OMR::JitBuilder::BytecodeBuilder *first = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	OMR::JitBuilder::BytecodeBuilder *last = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));

	truePath->AddSuccessorBuilder(&first);
	truePath->AppendBuilder(first);
	truePath->AddSuccessorBuilder(&last);
	truePath->AppendBuilder(last);

	generateGenericMethodBody(&first, genericSend, mergeSend, codeBlock, self2, bytecodeIndex, recursiveLevel);

	first->Goto(last);
	/* END OF BLOCK */

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForBooleanAndNoBlock(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *receiver = PICK(builder, 1);
	OMR::JitBuilder::IlValue *param1 = PICK(builder, 0);

	//IF receiver == falseObject then return false
	OMR::JitBuilder::BytecodeBuilder *falsePath = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	OMR::JitBuilder::BytecodeBuilder *truePath = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	builder->AddSuccessorBuilder(&falsePath);
	builder->AddSuccessorBuilder(&truePath);

	OMR::JitBuilder::IlBuilder *fPath = (OMR::JitBuilder::IlBuilder *)falsePath;
	OMR::JitBuilder::IlBuilder *tPath = (OMR::JitBuilder::IlBuilder *)truePath;
	builder->IfThenElse(&fPath, &tPath,
	builder->	EqualTo(
	builder->		ConstInt64((int64_t)falseObject), receiver));

	falsePath->Store("sendResult", receiver);

	//should verify the receiver is a boolean

	//VERIFY That the parameter is a boolean
	OMR::JitBuilder::IlValue *objectClass2 =
	truePath->Call("getSuperClass", 1, param1);

	OMR::JitBuilder::BytecodeBuilder *failurePath = nullptr;
	truePath->IfCmpNotEqual(&failurePath, objectClass2, truePath->ConstInt64((int64_t)booleanClass));

	//failurePath->Call("printString", 1, failurePath->ConstInt64((int64_t)"boolean and and param is not a block\n"));
	SET_STACKTOP(failurePath, stackTopForErrorHandling[recursiveLevel]);
	failurePath->Goto(*genericSend);

	//TODO switch to proper verification that it is a block class

	truePath->Store("sendResult", param1);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForBooleanOr(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *receiver = PICK(builder, 1);
	OMR::JitBuilder::IlValue *param1 = PICK(builder, 0);

	VMMethod *codeBlock = blockMethods.top();
	blockMethods.pop();

	const char *arguements = getArgumentName(recursiveLevel);
//	const char *localsName = getLocalName(recursiveLevel);

	long numOfArgs = codeBlock->GetNumberOfArguments();

	builder->Store(arguements,
	builder->	CreateLocalArray((int32_t)numOfArgs, Int64));

	for (int32_t i = 0; i < (int32_t)numOfArgs; i++) {
		builder->StoreAt(
		builder->	IndexAt(pInt64,
		builder->		ConvertTo(pInt64,
		builder->			Load(arguements)),
		builder->		ConstInt32(i)),
					PICK(builder, numOfArgs - 1 - i));
	}

//	OMR::JitBuilder::IlValue *self = getSelf(builder);
//	OMR::JitBuilder::IlValue *self2 = builder->Load(getSelfName(recursiveLevel));
//	builder->Call("printObject", 3, self, self2, receiver);
	OMR::JitBuilder::IlValue *self2 = nullptr;
	auto search = blockToReceiverMap.find((const void *)param1);
	if(search != blockToReceiverMap.end()) {
		self2 = search->second;
	} else {
		int *x = 0;
		fprintf(stderr, "Did not find receiver for block %p\n", param1);
		*x = 0;
	}

#if SOM_METHOD_DEBUG
	fprintf(stderr, " or ");
#endif

	//TODO do I need locals?

	//IF receiver == falseObject then return false
	OMR::JitBuilder::BytecodeBuilder *falsePath = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	OMR::JitBuilder::BytecodeBuilder *truePath = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	builder->AddSuccessorBuilder(&falsePath);
	builder->AddSuccessorBuilder(&truePath);

	OMR::JitBuilder::IlBuilder *fPath = (OMR::JitBuilder::IlBuilder *)falsePath;
	OMR::JitBuilder::IlBuilder *tPath = (OMR::JitBuilder::IlBuilder *)truePath;
	builder->IfThenElse(&tPath, &fPath,
	builder->	EqualTo(
	builder->		ConstInt64((int64_t)trueObject), receiver));

//	OMR::JitBuilder::IlValue *objectClass =
//	truePath->Call("getClass", 1, receiver);

	OMR::JitBuilder::BytecodeBuilder *failurePath = nullptr;
//	truePath->IfCmpNotEqual(&failurePath, objectClass, truePath->ConstInt64((int64_t)booleanClass));
//
//	//failurePath->Call("printString", 1, failurePath->ConstInt64((int64_t)"boolean and and param is not a block\n"));
//	SET_STACKTOP(failurePath, stackTopForErrorHandling[recursiveLevel]);
//	failurePath->Goto(*genericSend);

	truePath->Store("sendResult", receiver);

	//should verify the receiver is a boolean

//	fprintf(stderr, "generateInlineForBooleanOr\n");

	//VERIFY That the parameter is a block
	OMR::JitBuilder::IlValue *objectClass2 =
	falsePath->Call("getSuperClass", 1, param1);

	falsePath->IfCmpNotEqual(&failurePath, objectClass2, falsePath->ConstInt64((int64_t)blockClass));

	failurePath->Call("printString", 1, failurePath->ConstInt64((int64_t)"boolean or: and param is not a block\n"));
	SET_STACKTOP(failurePath, stackTopForErrorHandling[recursiveLevel]);
	failurePath->Goto(*genericSend);

	//TODO switch to proper verification that it is a block class



	/* START OF BLOCK */

	OMR::JitBuilder::BytecodeBuilder *first = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	OMR::JitBuilder::BytecodeBuilder *last = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));

	falsePath->AddSuccessorBuilder(&first);
	falsePath->AppendBuilder(first);
	falsePath->AddSuccessorBuilder(&last);
	falsePath->AppendBuilder(last);

	generateGenericMethodBody(&first, genericSend, mergeSend, codeBlock, self2, bytecodeIndex, recursiveLevel);

	first->Goto(last);
	/* END OF BLOCK */

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForBooleanOrNoBlock(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *receiver = PICK(builder, 1);
	OMR::JitBuilder::IlValue *param1 = PICK(builder, 0);

	//IF receiver == falseObject then return false
	OMR::JitBuilder::BytecodeBuilder *falsePath = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	OMR::JitBuilder::BytecodeBuilder *truePath = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	builder->AddSuccessorBuilder(&falsePath);
	builder->AddSuccessorBuilder(&truePath);

	OMR::JitBuilder::IlBuilder *fPath = (OMR::JitBuilder::IlBuilder *)falsePath;
	OMR::JitBuilder::IlBuilder *tPath = (OMR::JitBuilder::IlBuilder *)truePath;
	builder->IfThenElse(&tPath, &fPath,
	builder->	EqualTo(
	builder->		ConstInt64((int64_t)trueObject), receiver));

	truePath->Store("sendResult", receiver);

#if SOM_METHOD_DEBUG
	fprintf(stderr, " or no block ");
#endif

	//should verify the receiver is a boolean

	//VERIFY That the parameter is a boolean
	OMR::JitBuilder::IlValue *objectClass2 =
	falsePath->Call("getSuperClass", 1, param1);

	OMR::JitBuilder::BytecodeBuilder *failurePath = nullptr;
	falsePath->IfCmpNotEqual(&failurePath, objectClass2, falsePath->ConstInt64((int64_t)booleanClass));

	OMR::JitBuilder::BytecodeBuilder *failurePath2 = nullptr;
	failurePath->IfCmpEqual(&failurePath2, objectClass2, failurePath->ConstInt64((int64_t)blockClass));

//	failurePath2->Call("printString", 1, failurePath2->ConstInt64((int64_t)"boolean or: and param is not a boolean but it is a block\n"));
	SET_STACKTOP(failurePath2, stackTopForErrorHandling[recursiveLevel]);
	failurePath2->Goto(*genericSend);

//	failurePath->Call("printString", 1, failurePath->ConstInt64((int64_t)"boolean or: and param is not a boolean\n"));
	SET_STACKTOP(failurePath, stackTopForErrorHandling[recursiveLevel]);
	failurePath->Goto(*genericSend);

	//TODO switch to proper verification that it is a block class

	falsePath->Store("sendResult", param1);

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineForBooleanNot(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel)
{
	OMR::JitBuilder::IlValue *receiver = PICK(builder, 0);

	verifyBooleanArg(builder, genericSend, receiver, recursiveLevel);

	OMR::JitBuilder::BytecodeBuilder *falsePath = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	OMR::JitBuilder::BytecodeBuilder *truePath = OrphanBytecodeBuilder(bytecodeIndex, Bytecode::GetBytecodeName(BC_SEND));
	builder->AddSuccessorBuilder(&falsePath);
	builder->AddSuccessorBuilder(&truePath);

	OMR::JitBuilder::IlBuilder *fPath = (OMR::JitBuilder::IlBuilder *)falsePath;
	OMR::JitBuilder::IlBuilder *tPath = (OMR::JitBuilder::IlBuilder *)truePath;
	builder->IfThenElse(&tPath, &fPath,
	builder->	EqualTo(
	builder->		ConstInt64((int64_t)trueObject), receiver));

	falsePath->Store("sendResult", falsePath->ConstInt64((int64_t)trueObject));

	truePath->Store("sendResult", truePath->ConstInt64((int64_t)falseObject));

	return INLINE_SUCCESSFUL;
}

SOMppMethod::INLINE_STATUS
SOMppMethod::generateInlineOSRToGenericSend(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, long bytecodeIndex, int32_t recursiveLevel)
{
	OMR::JitBuilder::BytecodeBuilder *failurePath = nullptr;
	builder->IfCmpEqualZero(&failurePath,
	builder->		ConstInt64(0x0));

	SET_STACKTOP(failurePath, stackTopForErrorHandling[recursiveLevel]);
	failurePath->Goto(*genericSend);

//	fprintf(stderr, "SOMppMethod::generateInlineOSRToGenericSend recursiveLevel %d\n", recursiveLevel);

	return INLINE_SUCCESSFUL;
}

bool
SOMppMethod::methodIsInlineable(VMMethod *vmMethod, int32_t recursiveLevel)
{
	long bytecodeCount = vmMethod->GetNumberOfBytecodes();
	long i = 0;
	bool canInline = true;

	if (0 == strcmp(method->GetSignature()->GetChars(), vmMethod->GetSignature()->GetChars())) {
		// can not inline recursively
		return false;
	}

	if (recursiveLevel > 4) {
		fprintf(stderr, "method inlining failed at recursive depth %d\n", recursiveLevel);
		// can not currently handle more than an inline of depth 4
		return false;
	}

	uint32_t blockMethodsSize = blockMethods.size();
	bool didPush = false;

#if SOM_METHOD_DEBUG1
	fprintf(stderr, "\n\t\tmethodIsInlineable %s recLevel %d args%ld locals%ld\n", vmMethod->GetSignature()->GetChars(), recursiveLevel, vmMethod->GetNumberOfArguments(), vmMethod->GetNumberOfLocals());
#endif

	while ((i < bytecodeCount) && canInline) {
		uint8_t bc = vmMethod->GetBytecode(i);
#if SOM_METHOD_DEBUG1
		fprintf(stderr, "\t\t\tinline %ld %s", i, Bytecode::GetBytecodeName(bc));
#endif
		switch(bc) {
		case BC_DUP:
			break;
		case BC_PUSH_FIELD:
		{
#if SOM_METHOD_DEBUG1
			fprintf(stderr, " %d ", vmMethod->GetBytecode(i + 1));
#endif
			break;
		}
		case BC_PUSH_CONSTANT:
		case BC_PUSH_GLOBAL:
		case BC_POP:
		case BC_POP_ARGUMENT:
		case BC_POP_FIELD:
			break;
		case BC_PUSH_BLOCK:
		{
			VMMethod *blockMethod = static_cast<VMMethod*>(vmMethod->GetConstant(i));
			blockMethods.push(blockMethod);
			didPush = true;
			break;
		}
		case BC_JUMP_IF_FALSE:
		case BC_JUMP_IF_TRUE:
		case BC_JUMP:
		{
#if SOM_METHOD_DEBUG1
			fprintf(stderr, " %ld ", calculateBytecodeIndexForJump(vmMethod, i));
#endif
			break;
		}
		case BC_RETURN_LOCAL:
			break;
		case BC_RETURN_NON_LOCAL:
		{
//			if (recursiveLevel == 1) {
//				fprintf(stderr, " recrusive lvel %d ", recursiveLevel);
//			}
//			if (recursiveLevel > 1) {
//				canInline = false;
//			}
			if (recursiveLevel > 0) {
				fprintf(stderr, "failed to inline due to return non local recursive level %d\n", recursiveLevel);
				canInline = false;
			}
			break;
		}
		case BC_PUSH_ARGUMENT:
		{
#if SOM_METHOD_DEBUG1
			fprintf(stderr, " %d %d ", vmMethod->GetBytecode(i + 1), vmMethod->GetBytecode(i + 2));
#endif
			uint8_t level = vmMethod->GetBytecode(i + 2);
			if (level > 2) {
				fprintf(stderr, "failed to inline due to push argument level %d\n", level);
				canInline = false;
			}
			break;
		}
		case BC_PUSH_LOCAL:
		{
#if SOM_METHOD_DEBUG1
			fprintf(stderr, " %d %d ", vmMethod->GetBytecode(i + 1), vmMethod->GetBytecode(i + 2));
#endif
			uint8_t level = vmMethod->GetBytecode(i + 2);
			if (level > 2) {
				fprintf(stderr, "failed to inline due to push local level %d\n", level);
				canInline = false;
			}
			break;
		}
		case BC_POP_LOCAL:
		{
#if SOM_METHOD_DEBUG1
			fprintf(stderr, " %d %d ", vmMethod->GetBytecode(i + 1), vmMethod->GetBytecode(i + 2));
#endif
			uint8_t level = vmMethod->GetBytecode(i + 2);
			if (level > 2) {
				fprintf(stderr, "failed to inline due to pop local level %d\n", level);
				canInline = false;
			}
			break;
		}
		case BC_SEND:
		{
			VMSymbol* signature = static_cast<VMSymbol*>(vmMethod->GetConstant(i));
#if SOM_METHOD_DEBUG1
			fprintf(stderr, " %s", signature->GetChars());
#endif
			VMClass *receiverFromCache = vmMethod->getInvokeReceiverCache(i);
			if (nullptr != receiverFromCache) {
				VMClass *invokableClass = receiverFromCache->LookupInvokable(signature)->GetHolder();
				SOMppMethod::RECOGNIZED_METHOD_INDEX recognizedMethodIndex = getRecognizedMethodIndex(vmMethod, receiverFromCache, invokableClass, signature->GetChars(), recursiveLevel + 1);
				if (NOT_RECOGNIZED == recognizedMethodIndex) {
					if (NOT_RECOGNIZED == getRecognizedMethodIndex(vmMethod, receiverFromCache, invokableClass, signature->GetChars(), recursiveLevel + 1, false)) {
						VMInvokable *invokable = receiverFromCache->LookupInvokable(signature);
						if (nullptr == invokable) {
							//no invokable so it can not be inlined
							fprintf(stderr, "failed to inline due to null invokable %s cacheName %s\n", invokable->GetSignature()->GetChars(), receiverFromCache->GetName()->GetChars());
							canInline = false;
						} else if (!invokable->IsPrimitive()) {
							VMMethod *methodToInline = static_cast<VMMethod*>(invokable);
							if (!methodIsInlineable(methodToInline, recursiveLevel + 1)) {
								canInline = false;
							} else {
								//method will be inlined
							}
						} else {
							fprintf(stderr, "failed to inline due to send to primitive %s:>>%s\n", receiverFromCache->GetName()->GetChars(), invokable->GetSignature()->GetChars());
							canInline = false;
						}
					} else {
						fprintf(stderr, "failed to inline due to recognized method which is missing blocks? %s\n", signature->GetChars());
						canInline = false;
					}
				} else {
					//recognized method will be inlined
				}
			}
			/* If receiverFromCache is NULL generate an OSR point back to the generic send of the method which is inling this method */
			break;
		}
		case BC_SUPER_SEND:
		{
			fprintf(stderr, "failed to inline due to super send\n");
			canInline = false;
			break;
		}
		default:
			fprintf(stderr, "failed to inline due to bytecode %s", Bytecode::GetBytecodeName(bc));
			canInline = false;
		}
#if SOM_METHOD_DEBUG1
		fprintf(stderr, "\n");
#endif
		i += Bytecode::GetBytecodeLength(bc);
	}
	if (didPush) {
		if (blockMethods.size() > blockMethodsSize) {
//			fprintf(stderr, " pushed blocks that were not used! %u > %u\n", blockMethods.size(), blockMethodsSize);
			while (blockMethods.size() > blockMethodsSize) {
				blockMethods.pop();
			}
		}
		//canInline = false;
	}
#if SOM_METHOD_DEBUG1
	if (canInline) {
		fprintf(stderr, "\t\tsuccessfully inlined\n");
	} else {
		fprintf(stderr, "\t\tfailed to inline\n");
	}
#endif
	return canInline;
}

OMR::JitBuilder::IlValue *
SOMppMethod::getIntegerValue(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *object)
{
	OMR::JitBuilder::IlValue *value = nullptr;
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

OMR::JitBuilder::IlValue *
SOMppMethod::newIntegerObjectForValue(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::IlValue *value, int32_t recursiveLevel)
{
#if USE_TAGGING
	OMR::JitBuilder::BytecodeBuilder *failurePath = nullptr;
	builder->IfCmpLessThan(&failurePath, value,
	builder->	ConstInt64((int64_t)VMTAGGEDINTEGER_MIN));

	builder->IfCmpGreaterThan(&failurePath, value,
	builder->	ConstInt64((int64_t)VMTAGGEDINTEGER_MAX));

	SET_STACKTOP(failurePath, stackTopForErrorHandling[recursiveLevel]);
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

OMR::JitBuilder::IlValue *
SOMppMethod::getDoubleValue(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *object)
{
	builder->Store("doubleSlot",
	builder->	Add(object,
	builder->		ConstInt64((int64_t)(sizeof(int64_t)+sizeof(size_t)))));

	/* Read value */
	OMR::JitBuilder::IlValue *value =
	builder->LoadAt(pDouble,
	builder->	ConvertTo(pDouble,
	builder->		Load("doubleSlot")));

	return value;
}

OMR::JitBuilder::IlValue *
SOMppMethod::getDoubleValueFromDoubleOrInteger(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *object, OMR::JitBuilder::IlValue *objectClass)
{
	OMR::JitBuilder::IlBuilder *isDouble = nullptr;
	OMR::JitBuilder::IlBuilder *notDouble = nullptr;
	builder->IfThenElse(&isDouble, &notDouble,
	builder->	EqualTo(objectClass, builder->ConstInt64((int64_t)doubleClass)));

	isDouble->Store("doubleValue", getDoubleValue(isDouble, object));

	notDouble->Store("doubleValue", notDouble->ConvertTo(Double, getIntegerValue(notDouble, object)));

	return builder->Load("doubleValue");
}

OMR::JitBuilder::IlValue *
SOMppMethod::getIndexableFieldSlot(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *array, OMR::JitBuilder::IlValue *index)
{
	OMR::JitBuilder::IlValue *indexValue = getIntegerValue(builder, index);
	OMR::JitBuilder::IlValue *numberOfFields = builder->LoadIndirect("VMObject", "numberOfFields", array);
	OMR::JitBuilder::IlValue *vmObjectSize = builder->ConstInt64(sizeof(VMObject));
	OMR::JitBuilder::IlValue *vmObjectPointerSize = builder->ConstInt64(sizeof(VMObject*));
	OMR::JitBuilder::IlValue *indexableIndex = builder->Add(indexValue, numberOfFields);

	OMR::JitBuilder::IlValue *actualIndex =
	builder->Sub(indexableIndex,
	builder->	ConstInt64(1));

	OMR::JitBuilder::IlValue *offset =
	builder->Add(vmObjectSize,
	builder->	Mul(actualIndex, vmObjectPointerSize));

	return builder->Add(array, offset);
}

SOMppMethod::RECOGNIZED_METHOD_INDEX
SOMppMethod::getRecognizedMethodIndex(VMMethod *sendingMethod, VMClass *receiverFromCache, VMClass *invokableClass, const char *signatureChars, int32_t recursiveLevel, bool doBlockInliningChecks)
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
			//if (0 != strcmp("to:do:", sendingMethod->GetSignature()->GetChars())) {
				if (doBlockInliningChecks) {
					if (doLoopInlining && !blockMethods.empty()) {
						forLoopBlock = blockMethods.top();
						VMMethod *block = forLoopBlock;
						blockMethods.pop();
						if (methodIsInlineable(forLoopBlock, recursiveLevel)) {
							blockMethods.push(block);
							return INTEGER_TOBYDO;
						}
					}
				} else {
					return INTEGER_TOBYDO;
				}
			//}
		} else if (0 == strcmp("to:do:", signatureChars)){
			if (doBlockInliningChecks) {
				if (doLoopInlining && !blockMethods.empty()) {
					forLoopBlock = blockMethods.top();
					VMMethod *block = forLoopBlock;
					blockMethods.pop();
					if (methodIsInlineable(forLoopBlock, recursiveLevel)) {
						blockMethods.push(block);
						return INTEGER_TODO;
					}
				}
			} else {
				return INTEGER_TODO;
			}
		} else if (0 == strcmp("downTo:by:do:", signatureChars)){
			//if (0 != strcmp("downTo:do:", sendingMethod->GetSignature()->GetChars())) {
				if (doBlockInliningChecks) {
					if (doLoopInlining && !blockMethods.empty()) {
						forLoopBlock = blockMethods.top();
						VMMethod *block = forLoopBlock;
						blockMethods.pop();
						if (methodIsInlineable(forLoopBlock, recursiveLevel)) {
							blockMethods.push(block);
							return INTEGER_DOWNTOBYDO;
						}
					}
				} else {
					return INTEGER_DOWNTOBYDO;
				}
			//}
		} else if (0 == strcmp("downTo:do:", signatureChars)){
			if (doBlockInliningChecks) {
				if (doLoopInlining && !blockMethods.empty()) {
					forLoopBlock = blockMethods.top();
					VMMethod *block = forLoopBlock;
					blockMethods.pop();
					if (methodIsInlineable(forLoopBlock, recursiveLevel)) {
						blockMethods.push(block);
						return INTEGER_DOWNTODO;
					}
				}
			} else {
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
		} else if (0 == strcmp("do:", signatureChars)) {
			if (doBlockInliningChecks) {
				if (doLoopInlining && !blockMethods.empty()) {
					forLoopBlock = blockMethods.top();
					VMMethod *block = forLoopBlock;
					blockMethods.pop();
					if (methodIsInlineable(forLoopBlock, recursiveLevel)) {
						blockMethods.push(block);
						return ARRAY_DO;
					}
				}
			} else {
				return ARRAY_DO;
			}
		} else if (0 == strcmp("doIndexes:", signatureChars)) {
			if (doBlockInliningChecks) {
				if (doLoopInlining && !blockMethods.empty()) {
					forLoopBlock = blockMethods.top();
					VMMethod *block = forLoopBlock;
					blockMethods.pop();
					if (methodIsInlineable(forLoopBlock, recursiveLevel)) {
						blockMethods.push(block);
						return ARRAY_DOINDEXES;
					}
				}
			} else {
				return ARRAY_DOINDEXES;
			}
		}
	} else if (((VMClass*)arrayClass)->GetClass() == invokableClass) {
		if (0 == strcmp("new:", signatureChars)) {
			return ARRAY_NEW;
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
			//char *sendMethodChars = sendingMethod->GetSignature()->GetChars();
			//if ((0 != strcmp("whileFalse:", sendMethodChars)) && (0 != strcmp("to:by:do:", sendMethodChars)) && (0 != strcmp("downTo:by:do:", sendMethodChars))) {
				if (doBlockInliningChecks) {
					if (doLoopInlining && (blockMethods.size() > 1)) {
						whileLoopCodeBlock = blockMethods.top();
						VMMethod *codeBlock = whileLoopCodeBlock;
						blockMethods.pop();
						whileLoopConditionBlock = blockMethods.top();
						VMMethod *conditionBlock = whileLoopConditionBlock;
						blockMethods.pop();
						if (methodIsInlineable(whileLoopConditionBlock, recursiveLevel) && methodIsInlineable(whileLoopCodeBlock, recursiveLevel)) {
							blockMethods.push(conditionBlock);
							blockMethods.push(codeBlock);
							return BLOCK_WHILETRUE;
						}
					}
				} else {
					return BLOCK_WHILETRUE;
				}
			//}
		} else if (0 == strcmp("whileFalse:", signatureChars)) {
			if (doBlockInliningChecks) {
				if (doLoopInlining && (blockMethods.size() > 1)) {
					whileLoopCodeBlock = blockMethods.top();
					VMMethod *codeBlock = whileLoopCodeBlock;
					blockMethods.pop();
					whileLoopConditionBlock = blockMethods.top();
					VMMethod *conditionBlock = whileLoopConditionBlock;
					blockMethods.pop();
					if (methodIsInlineable(whileLoopConditionBlock, recursiveLevel) && methodIsInlineable(whileLoopCodeBlock, recursiveLevel)) {
						blockMethods.push(conditionBlock);
						blockMethods.push(codeBlock);
						return BLOCK_WHILEFALSE;
					}
				}
			} else {
				return BLOCK_WHILEFALSE;
			}
		}
	} else if ((((VMClass*)trueClass == receiverFromCache) || ((VMClass*)falseClass == receiverFromCache))) {
		if (0 == strcmp("&&", signatureChars)) {
			if (!blockMethods.empty()) {
				VMMethod *codeBlock = blockMethods.top();
				blockMethods.pop();
				if (methodIsInlineable(codeBlock, recursiveLevel)) {
					blockMethods.push(codeBlock);
					return BOOLEAN_AND;
				}
			} else {
				return BOOLEAN_AND_NOBLOCK;
			}
		} else if ((0 == strcmp("and:", signatureChars)) && (0 != strcmp("&&", sendingMethod->GetSignature()->GetChars()))) {
			if (!blockMethods.empty()) {
				VMMethod *codeBlock = blockMethods.top();
				blockMethods.pop();
				if (methodIsInlineable(codeBlock, recursiveLevel)) {
					blockMethods.push(codeBlock);
					return BOOLEAN_AND;
				}
			} else {
				return BOOLEAN_AND_NOBLOCK;
			}
		} else if (0 == strcmp("||", signatureChars)) {
			if (!blockMethods.empty()) {
				VMMethod *codeBlock = blockMethods.top();
				blockMethods.pop();
				if (methodIsInlineable(codeBlock, recursiveLevel)) {
					blockMethods.push(codeBlock);
					return BOOLEAN_OR;
				}
			} else {
				return BOOLEAN_OR_NOBLOCK;
			}
		} else if ((0 == strcmp("or:", signatureChars)) && (0 != strcmp("||", sendingMethod->GetSignature()->GetChars()))) {
			if (!blockMethods.empty()) {
				VMMethod *codeBlock = blockMethods.top();
				blockMethods.pop();
				if (methodIsInlineable(codeBlock, recursiveLevel)) {
					blockMethods.push(codeBlock);
					return BOOLEAN_OR;
				}
			} else {
				return BOOLEAN_OR_NOBLOCK;
			}
		} else if ((0 == strcmp("not", signatureChars))) {
			return BOOLEAN_NOT;
		}
	}
	return NOT_RECOGNIZED;
}
