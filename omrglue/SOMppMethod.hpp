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

#include <stack>

class VMBlock;
class VMMethod;
class VMSymbol;
class VMClass;
class VMInvokable;
typedef int64_t (SOMppFunctionType)(int64_t interpreter, int64_t frame);


typedef TR::IlValue * (*MathFuncType)(TR::BytecodeBuilder *builder, TR::IlValue *param1, TR::IlValue *param2);
typedef TR::IlValue * (*BooleanFuncType)(TR::BytecodeBuilder *builder, TR::IlValue *param1, TR::IlValue *param2);
typedef void (*ForLoopFuncType)(TR::BytecodeBuilder *builder, const char *index, TR::IlBuilder **loop, TR::IlValue *start, TR::IlValue *end, TR::IlValue *increment);

#define FIELDNAMES_LENGTH 10
#define STACKVALUEILTYPE Int64
#define	STACKVALUETYPE int64_t

#define MAX_RECURSIVE_INLINING_DEPTH 4

class SOMppMethod: public TR::MethodBuilder {
	enum INLINE_STATUS {
		INLINE_FAILED,
		INLINE_SUCCESSFUL,
		INLINE_SUCCESSFUL_NO_GENERIC_PATH_REQUIRED
	};
	enum RECOGNIZED_METHOD_INDEX {
		OBJECT_EQUAL = 0,
		OBJECT_NOTEQUAL,
		OBJECT_VALUE,
		GENERIC_ISNIL,
		GENERIC_NOTNIL,
		INTEGER_PLUS,
		INTEGER_MINUS,
		INTEGER_MULTIPLY,
		INTEGER_DIVIDE,
		INTEGER_PERCENT,
		INTEGER_AND,
		INTEGER_LESSTHAN,
		INTEGER_LESSTHANEQUAL,
		INTEGER_GREATERTHAN,
		INTEGER_GREATERTHANEQUAL,
		INTEGER_EQUAL,
		INTEGER_NOTEQUAL,
		INTEGER_NEGATED,
		INTEGER_MAX,
		INTEGER_ABS,
		INTEGER_TODO,
		INTEGER_TOBYDO,
		INTEGER_DOWNTODO,
		INTEGER_DOWNTOBYDO,
		ARRAY_AT,
		ARRAY_ATPUT,
		ARRAY_LENGTH,
		DOUBLE_PLUS,
		DOUBLE_MINUS,
		DOUBLE_MULTIPLY,
		DOUBLE_DIVIDE,
		DOUBLE_LESSTHAN,
		DOUBLE_LESSTHANEQUAL,
		DOUBLE_GREATERTHAN,
		DOUBLE_GREATERTHANEQUAL,
		BLOCK_WHILETRUE,
		BLOCK_WHILEFALSE,
		BOOLEAN_AND,
		BOOLEAN_OR,
		OSR_TO_GENERIC_SEND,
		NOT_RECOGNIZED
	};
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

	static TR::IlValue *add(TR::BytecodeBuilder *builder, TR::IlValue *param1, TR::IlValue *param2);
	static TR::IlValue *sub(TR::BytecodeBuilder *builder, TR::IlValue *param1, TR::IlValue *param2);
	static TR::IlValue *mul(TR::BytecodeBuilder *builder, TR::IlValue *param1, TR::IlValue *param2);
	static TR::IlValue *div(TR::BytecodeBuilder *builder, TR::IlValue *param1, TR::IlValue *param2);
	static TR::IlValue *percent(TR::BytecodeBuilder *builder, TR::IlValue *param1, TR::IlValue *param2);
	static TR::IlValue *andVals(TR::BytecodeBuilder *builder, TR::IlValue *param1, TR::IlValue *param2);
	static TR::IlValue *lessThan(TR::BytecodeBuilder *builder, TR::IlValue *param1, TR::IlValue *param2);
	static TR::IlValue *greaterThan(TR::BytecodeBuilder *builder, TR::IlValue *param1, TR::IlValue *param2);
	static TR::IlValue *equalTo(TR::BytecodeBuilder *builder, TR::IlValue *param1, TR::IlValue *param2);
	static void forLoopUp(TR::BytecodeBuilder *builder, const char *index, TR::IlBuilder **loop, TR::IlValue *start, TR::IlValue *end, TR::IlValue *increment);
	static void forLoopDown(TR::BytecodeBuilder *builder, const char *index, TR::IlBuilder **loop, TR::IlValue *start, TR::IlValue *end, TR::IlValue *increment);
private:
	VMMethod *method;
	std::stack<VMMethod *> blockMethods;
	VMMethod *forLoopBlock;
	VMMethod *whileLoopConditionBlock;
	VMMethod *whileLoopCodeBlock;
	int32_t stackTopForErrorHandling[MAX_RECURSIVE_INLINING_DEPTH + 1];
	int32_t extraStackDepthRequired;
	/* Should inlining be attempted */
	bool doInlining;
	bool doLoopInlining;
	/* Provide a name for the method */
	char methodName[64];

	void defineFunctions();
	void defineStructures(TR::TypeDictionary *types);
	void defineLocals();
	void defineParameters();

	void defineVMFrameStructure(TR::TypeDictionary *types);
	void defineVMObjectStructure(TR::TypeDictionary *types);

	void justReturn(TR::IlBuilder *from);

	TR::BytecodeBuilder **createBytecodesBuilder(VMMethod *vmMethod);
	int64_t calculateBytecodeIndexForJump(VMMethod *vmMethod, long bytecodeIndex);
	bool generateILForBytecodes(VMMethod *vmMethod, TR::BytecodeBuilder **bytecodeBuilderTable);

	void doDup(TR::BytecodeBuilder *builder);
	void doPushLocal(TR::BytecodeBuilder *builder, long bytecodeIndex);
	void doPushArgument(TR::BytecodeBuilder *builder, long bytecodeIndex);
	void doPushField(TR::BytecodeBuilder *builder, long bytecodeIndex);
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
	TR::IlValue *getSelf(TR::IlBuilder *builder);
	void pushField(TR::BytecodeBuilder *builder, TR::IlValue *object, uint8_t fieldIndex);
	void pushConstant(TR::BytecodeBuilder *builder, VMMethod *vmMethod, uint8_t valueOffset);
	void pushValueFromArray(TR::BytecodeBuilder *builder, TR::IlValue *array, uint8_t arrayIndex);
	void pushGlobal(TR::BytecodeBuilder *builder, VMSymbol* globalName);
	void popField(TR::BytecodeBuilder *builder, TR::IlValue *object, uint8_t fieldIndex);
	void popValueToArray(TR::BytecodeBuilder *builder, TR::IlValue *array, uint8_t arrayIndex);
	TR::IlValue *getLocalArrayForLevel(TR::BytecodeBuilder *builder, VMMethod *vmMethod, long bytecodeIndex, int32_t recursiveLevel);
	TR::IlValue *getArgumentArrayForLevel(TR::BytecodeBuilder *builder, VMMethod *vmMethod, long bytecodeIndex, int32_t recursiveLevel);

	SOMppMethod::INLINE_STATUS doInlineIfPossible(TR::BytecodeBuilder **builder, TR::BytecodeBuilder **genericSend, TR::BytecodeBuilder **merge, VMSymbol *signature, long bytecodeIndex);
	SOMppMethod::INLINE_STATUS generateRecognizedMethod(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, TR::BytecodeBuilder **merge, SOMppMethod::RECOGNIZED_METHOD_INDEX recognizedMethodIndex, VMClass *receiverFromCache, long bytecodeIndex, int32_t recursiveLevel);
	void generateGenericMethod(TR::BytecodeBuilder **builder, TR::BytecodeBuilder **genericSend, TR::BytecodeBuilder **merge, VMInvokable *invokable, VMClass *receiverClass, VMSymbol *signature, long bytecodeIndex, int32_t recursiveLevel = 0);
	void generateGenericMethodBody(TR::BytecodeBuilder **builder, TR::BytecodeBuilder **genericSend, TR::BytecodeBuilder **merge, VMMethod *methodToInline, TR::IlValue *receiver, long bytecodeIndex, int32_t recursiveLevel = 0);
	void createBuildersForInlineSends(TR::BytecodeBuilder **genericSend, TR::BytecodeBuilder **merge, long bytecodeIndex);
	bool methodIsInlineable(VMMethod *vmMethod, int32_t recursiveLevel);

	SOMppMethod::INLINE_STATUS generateInlineForObjectNotEqual(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend,VMClass *receiverClass);
	SOMppMethod::INLINE_STATUS generateInlineForObjectEqual(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, VMClass *receiverClass);
	SOMppMethod::INLINE_STATUS generateInlineForObjectValue(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, VMClass *receiverClass);

	SOMppMethod::INLINE_STATUS generateInlineForGenericIsNil(TR::BytecodeBuilder *builder);
	SOMppMethod::INLINE_STATUS generateInlineForGenericNotNil(TR::BytecodeBuilder *builder);

	SOMppMethod::INLINE_STATUS generateInlineForIntegerMath(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, MathFuncType mathFunction);
	SOMppMethod::INLINE_STATUS generateInlineForIntegerBoolean(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, BooleanFuncType booleanFunction, TR::IlValue *thenPathValue, TR::IlValue *elsePathValue);
	SOMppMethod::INLINE_STATUS generateInlineForIntegerNegated(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend);
	SOMppMethod::INLINE_STATUS generateInlineForIntegerMax(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend);
	SOMppMethod::INLINE_STATUS generateInlineForIntegerAbs(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, long bytecodeIndex);

	void generateInlineForIntegerLoop(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, TR::BytecodeBuilder **mergeSend, long bytecodeIndex, ForLoopFuncType loopFunction, TR::IlValue *start, TR::IlValue *end, TR::IlValue *increment, TR::IlValue *block, VMMethod *blockToInline, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForIntegerToDo(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, TR::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForIntegerToByDo(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, TR::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForIntegerDownToDo(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, TR::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForIntegerDownToByDo(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, TR::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel);

	SOMppMethod::INLINE_STATUS generateInlineForArrayAt(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend);
	SOMppMethod::INLINE_STATUS generateInlineForArrayAtPut(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend);
	SOMppMethod::INLINE_STATUS generateInlineForArrayLength(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend);

	SOMppMethod::INLINE_STATUS generateInlineForDoubleMath(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, MathFuncType mathFunction);
	SOMppMethod::INLINE_STATUS generateInlineForDoubleBoolean(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, BooleanFuncType booleanFunction, TR::IlValue *thenPathValue, TR::IlValue *elsePathValue);

	void generateForWhileLoop(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, TR::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel, TR::IlValue *condition);
	SOMppMethod::INLINE_STATUS generateInlineForWhileTrue(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, TR::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForWhileFalse(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, TR::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel);

	SOMppMethod::INLINE_STATUS generateInlineForBooleanAnd(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, long bytecodeIndex);
	SOMppMethod::INLINE_STATUS generateInlineForBooleanOr(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, long bytecodeIndex);

	SOMppMethod::INLINE_STATUS generateInlineOSRToGenericSend(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, long bytecodeIndex);

	void verifyIntegerArg(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, TR::IlValue *value);
	void verifyDoubleArg(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, TR::IlValue *object, TR::IlValue *objectClass);
	void verifyArg(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, TR::IlValue *object, TR::IlValue *type);
	void verifyBooleanArg(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, TR::IlValue *object);
	TR::IlValue *getIntegerValue(TR::IlBuilder *builder, TR::IlValue *object);
	TR::IlValue *newIntegerObjectForValue(TR::BytecodeBuilder *builder, TR::BytecodeBuilder **genericSend, TR::IlValue *value);
	TR::IlValue *getDoubleValue(TR::IlBuilder *builder, TR::IlValue *object);
	TR::IlValue *getDoubleValueFromDoubleOrInteger(TR::IlBuilder *builder, TR::IlValue *object, TR::IlValue *objectClass);
	TR::IlValue *getIndexableFieldSlot(TR::BytecodeBuilder *builder, TR::IlValue *array, TR::IlValue *index);

	SOMppMethod::RECOGNIZED_METHOD_INDEX getRecognizedMethodIndex(VMMethod *sendingMethod, VMClass *receiverFromCache, VMClass *invokableClass, const char *signatureChars, int32_t recursiveLevel, bool doBlockInliningChecks = true);
};

#endif // !defined(SOMPPMETHOD_INCL)
