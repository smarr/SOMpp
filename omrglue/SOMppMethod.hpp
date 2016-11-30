/*******************************************************************************
 *
 * (c) Copyright IBM Corp. 2000, 2016
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

#ifndef SOMPPMETHOD_INCL
#define SOMPPMETHOD_INCL

#include "ilgen/MethodBuilder.hpp"
#include "ilgen/VirtualMachineOperandStack.hpp"
#include "ilgen/VirtualMachineRegisterInStruct.hpp"

namespace TR {
class IlBuilder;
class BytecodeBuilder;
class TypeDictionary;
class IlType;
}

class VMMethod;
class VMSymbol;
class VMClass;
typedef int64_t (SOMppFunctionType)(int64_t interpreter, int64_t frame);

#define FIELDNAMES_LENGTH 10
#define STACKVALUEILTYPE Int64
#define	STACKVALUETYPE int64_t

class SOMppMethod: public TR::MethodBuilder {
public:
	SOMppMethod(TR::TypeDictionary *types, VMMethod *vmMethod, bool inlineCalls);
	virtual bool buildIL();
protected:
	TR::IlType *pInt64;
	TR::IlType *pDouble;
	TR::IlType *vmFrame;
	TR::IlType *pVMFrame;
	TR::IlType *vmObject;
	TR::IlType *valueType;
	OMR::VirtualMachineOperandStack *stack;
	OMR::VirtualMachineRegister *stackTop;
	const char * fieldNames[FIELDNAMES_LENGTH];
private:
	VMMethod *method;
	/* Should inlining be attempted */
	bool doInlining;
	/* This is used to check if a send can be inlined */
	long currentStackDepth;
	/* Provide a name for the method */
	char methodName[64];

	void defineFunctions();
	void defineStructures(TR::TypeDictionary *types);
	void defineLocals();
	void defineParameters();

	void defineVMFrameStructure(TR::TypeDictionary *types);
	void defineVMObjectStructure(TR::TypeDictionary *types);

	void justReturn(TR::IlBuilder *from);

	void createBuilderForBytecode(TR::BytecodeBuilder **bytecodeBuilderTable, uint8_t bytecode, int64_t bytecodeIndex);
	int64_t calculateBytecodeIndexForJump(long bytecodeIndex);
	bool generateILForBytecode(TR::BytecodeBuilder **bytecodeBuilderTable, uint8_t bytecode, long bytecodeIndex);

	void doDup(TR::BytecodeBuilder *builder);
	void doPushLocal(TR::BytecodeBuilder *builder, long bytecodeIndex);
	void doPushArgument(TR::BytecodeBuilder *builder, long bytecodeIndex);
	void doPushField(TR::BytecodeBuilder *builder, VMMethod *currentMethod, long bytecodeIndex);
	void doPushBlock(TR::BytecodeBuilder *builder, long bytecodeIndex);
	void doPushConstant(TR::BytecodeBuilder *builder, long bytecodeIndex);
	void doPushGlobal(TR::BytecodeBuilder *builder, long bytecodeIndex);
	void doPop(TR::BytecodeBuilder *builder);
	void doPopLocal(TR::BytecodeBuilder *builder, long bytecodeIndex);
	void doPopArgument(TR::BytecodeBuilder *builder, long bytecodeIndex);
	void doPopField(TR::BytecodeBuilder *builder, long bytecodeIndex);
	void doSend(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex, TR::BytecodeBuilder *fallThrough);
	void doSuperSend(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex);
	void doReturnLocal(TR::BytecodeBuilder *builder, long bytecodeIndex);
	void doReturnNonLocal(TR::BytecodeBuilder *builder, long bytecodeIndex);
	void doJumpIfFalse(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex);
	void doJumpIfTrue(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex);
	void doJump(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex);

	const char *getContext(TR::IlBuilder *builder, uint8_t level);
	TR::IlValue *getOuterContext(TR::IlBuilder *builder);
	TR::IlValue *getSelfFromContext(TR::IlBuilder *builder, TR::IlValue *context);
};

#endif // !defined(SOMPPMETHOD_INCL)
