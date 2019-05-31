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

#include "JitBuilder.hpp"
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

class SOMppMethod: public OMR::JitBuilder::MethodBuilder {
public:
	SOMppMethod(OMR::JitBuilder::TypeDictionary *types, VMMethod *vmMethod, bool inlineCalls);
	virtual bool buildIL();
protected:
	OMR::JitBuilder::IlType *pInt64;
	OMR::JitBuilder::IlType *pDouble;
	OMR::JitBuilder::IlType *vmFrame;
	OMR::JitBuilder::IlType *pVMFrame;
	OMR::JitBuilder::IlType *vmObject;
        OMR::JitBuilder::IlType *valueType;
	OMR::JitBuilder::VirtualMachineOperandStack *stack;
	OMR::JitBuilder::VirtualMachineRegister *stackTop;
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
	void defineStructures(OMR::JitBuilder::TypeDictionary *types);
	void defineLocals();
	void defineParameters();

	void defineVMFrameStructure(OMR::JitBuilder::TypeDictionary *types);
	void defineVMObjectStructure(OMR::JitBuilder::TypeDictionary *types);

	void justReturn(OMR::JitBuilder::IlBuilder *from);

	void createBuilderForBytecode(OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable, uint8_t bytecode, int64_t bytecodeIndex);
	int64_t calculateBytecodeIndexForJump(long bytecodeIndex);
	bool generateILForBytecode(OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable, uint8_t bytecode, long bytecodeIndex);

	void doDup(OMR::JitBuilder::BytecodeBuilder *builder);
	void doPushLocal(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex);
	void doPushArgument(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex);
	void doPushField(OMR::JitBuilder::BytecodeBuilder *builder, VMMethod *currentMethod, long bytecodeIndex);
	void doPushBlock(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex);
	void doPushConstant(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex);
	void doPushGlobal(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex);
	void doPop(OMR::JitBuilder::BytecodeBuilder *builder);
	void doPopLocal(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex);
	void doPopArgument(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex);
	void doPopField(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex);
	void doSend(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex);
	void doSuperSend(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex);
	void doReturnLocal(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex);
	void doReturnNonLocal(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex);
	void doJumpIfFalse(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex);
	void doJumpIfTrue(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex);
	void doJump(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex);

	OMR::JitBuilder::IlValue *peek(OMR::JitBuilder::IlBuilder *builder);
	void pop(OMR::JitBuilder::IlBuilder *builder);
	void push(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *value);
	const char *getContext(OMR::JitBuilder::IlBuilder *builder, uint8_t level);
	OMR::JitBuilder::IlValue *getOuterContext(OMR::JitBuilder::IlBuilder *builder);
	OMR::JitBuilder::IlValue *getSelfFromContext(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *context);
	int getReceiverForSend(OMR::JitBuilder::IlBuilder *builder, VMSymbol* signature);
	OMR::JitBuilder::IlValue *getNumberOfIndexableFields(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *array);
	void getIndexableFieldSlot(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *array);

	OMR::JitBuilder::IlBuilder *doInlineIfPossible(OMR::JitBuilder::BytecodeBuilder *builder, VMSymbol* signature, long bytecodeIndex);
	OMR::JitBuilder::IlBuilder *generateRecognizedMethod(OMR::JitBuilder::BytecodeBuilder *builder, VMClass *receiverFromCache, char *signatureChars);
	OMR::JitBuilder::IlBuilder *generateGenericInline(OMR::JitBuilder::BytecodeBuilder *builder, VMClass *receiverFromCache, VMMethod *vmMethod, char *signatureChars);
	bool methodIsInlineable(VMMethod *vmMethod);

	/* Generate IL for primitives and known simple methods */
	/* Integer methods */
	OMR::JitBuilder::IlBuilder *generateILForIntergerOps(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlBuilder **failPath);
	OMR::JitBuilder::IlBuilder *verifyIntegerObject(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *object, OMR::JitBuilder::IlBuilder **failPath);
	OMR::JitBuilder::IlBuilder *getIntegerValue(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *object, const char *valueName, OMR::JitBuilder::IlBuilder **failPath);
	void createNewInteger(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *integerValue);

	OMR::JitBuilder::IlBuilder *generateILForIntegerLessThan(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForIntegerLessThanEqual(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForIntegerGreaterThan(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForIntegerGreaterThanEqual(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForIntegerEqual(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForIntegerNotEqual(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForIntegerPlus(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForIntegerMinus(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForIntegerStar(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForIntegerPercent(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForIntegerValue(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForIntegerMax(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForIntegerNegated(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForIntegerAbs(OMR::JitBuilder::BytecodeBuilder *builder);

	/* Array methods */
	OMR::JitBuilder::IlBuilder *generateILForArrayAt(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForArrayAtPut(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForArrayLength(OMR::JitBuilder::BytecodeBuilder *builder);

	/* Nil methods */
	OMR::JitBuilder::IlBuilder *generateILForNilisNil(OMR::JitBuilder::BytecodeBuilder *builder);

	/* Boolean methods */
	OMR::JitBuilder::IlBuilder *generateILForBooleanNot(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForBooleanAnd(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForBooleanOr(OMR::JitBuilder::BytecodeBuilder *builder);

	/* Integer methods */
	OMR::JitBuilder::IlBuilder *generateILForDoubleOps(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlBuilder **failPath);
	OMR::JitBuilder::IlBuilder *generateILForDoubleLessThan(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForDoubleLessThanEqual(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForDoubleGreaterThan(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForDoubleGreaterThanEqual(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForDoubleEqual(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForDoubleNotEqual(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForDoublePlus(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForDoubleMinus(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForDoubleStar(OMR::JitBuilder::BytecodeBuilder *builder);
	OMR::JitBuilder::IlBuilder *generateILForDoubleSlashSlash(OMR::JitBuilder::BytecodeBuilder *builder);
};

#endif // !defined(SOMPPMETHOD_INCL)
