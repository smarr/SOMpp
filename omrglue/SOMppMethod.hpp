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
	void doSend(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex);
	void doSuperSend(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex);
	void doReturnLocal(TR::BytecodeBuilder *builder, long bytecodeIndex);
	void doReturnNonLocal(TR::BytecodeBuilder *builder, long bytecodeIndex);
	void doJumpIfFalse(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex);
	void doJumpIfTrue(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex);
	void doJump(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex);

	TR::IlValue *peek(TR::IlBuilder *builder);
	void pop(TR::IlBuilder *builder);
	void push(TR::IlBuilder *builder, TR::IlValue *value);
	const char *getContext(TR::IlBuilder *builder, uint8_t level);
	TR::IlValue *getOuterContext(TR::IlBuilder *builder);
	TR::IlValue *getSelfFromContext(TR::IlBuilder *builder, TR::IlValue *context);
	int getReceiverForSend(TR::IlBuilder *builder, VMSymbol* signature);
	TR::IlValue *getNumberOfIndexableFields(TR::IlBuilder *builder, TR::IlValue *array);
	void getIndexableFieldSlot(TR::IlBuilder *builder, TR::IlValue *array);

	TR::IlBuilder *doInlineIfPossible(TR::BytecodeBuilder *builder, VMSymbol* signature, long bytecodeIndex);
	TR::IlBuilder *generateRecognizedMethod(TR::BytecodeBuilder *builder, VMClass *receiverFromCache, char *signatureChars);
	TR::IlBuilder *generateGenericInline(TR::BytecodeBuilder *builder, VMClass *receiverFromCache, VMMethod *vmMethod, char *signatureChars);
	bool methodIsInlineable(VMMethod *vmMethod);

	/* Generate IL for primitives and known simple methods */
	/* Integer methods */
	TR::IlBuilder *generateILForIntergerOps(TR::BytecodeBuilder *builder, TR::IlBuilder **failPath);
	TR::IlBuilder *verifyIntegerObject(TR::IlBuilder *builder, TR::IlValue *object, TR::IlBuilder **failPath);
	TR::IlBuilder *getIntegerValue(TR::IlBuilder *builder, TR::IlValue *object, const char *valueName, TR::IlBuilder **failPath);
	void createNewInteger(TR::IlBuilder *builder, TR::IlValue *integerValue);

	TR::IlBuilder *generateILForIntegerLessThan(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForIntegerLessThanEqual(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForIntegerGreaterThan(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForIntegerGreaterThanEqual(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForIntegerEqual(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForIntegerNotEqual(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForIntegerPlus(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForIntegerMinus(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForIntegerStar(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForIntegerPercent(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForIntegerValue(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForIntegerMax(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForIntegerNegated(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForIntegerAbs(TR::BytecodeBuilder *builder);

	/* Array methods */
	TR::IlBuilder *generateILForArrayAt(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForArrayAtPut(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForArrayLength(TR::BytecodeBuilder *builder);

	/* Nil methods */
	TR::IlBuilder *generateILForNilisNil(TR::BytecodeBuilder *builder);

	/* Boolean methods */
	TR::IlBuilder *generateILForBooleanNot(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForBooleanAnd(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForBooleanOr(TR::BytecodeBuilder *builder);

	/* Integer methods */
	TR::IlBuilder *generateILForDoubleOps(TR::BytecodeBuilder *builder, TR::IlBuilder **failPath);
	TR::IlBuilder *generateILForDoubleLessThan(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForDoubleLessThanEqual(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForDoubleGreaterThan(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForDoubleGreaterThanEqual(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForDoubleEqual(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForDoubleNotEqual(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForDoublePlus(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForDoubleMinus(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForDoubleStar(TR::BytecodeBuilder *builder);
	TR::IlBuilder *generateILForDoubleSlashSlash(TR::BytecodeBuilder *builder);
};

#endif // !defined(SOMPPMETHOD_INCL)
