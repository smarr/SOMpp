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
#include "VirtualMachineOperandStack.hpp"
#include "VirtualMachineRegisterInStruct.hpp"

#include <stack>
#include <map>

class VMBlock;
class VMMethod;
class VMSymbol;
class VMClass;
class VMInvokable;
typedef int64_t (SOMppFunctionType)(int64_t interpreter, int64_t frame);


typedef OMR::JitBuilder::IlValue * (*MathFuncType)(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *param1, OMR::JitBuilder::IlValue *param2);
typedef OMR::JitBuilder::IlValue * (*BooleanFuncType)(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *param1, OMR::JitBuilder::IlValue *param2);
typedef void (*ForLoopFuncType)(OMR::JitBuilder::BytecodeBuilder *builder, const char *index, OMR::JitBuilder::IlBuilder **loop, OMR::JitBuilder::IlValue *start, OMR::JitBuilder::IlValue *end, OMR::JitBuilder::IlValue *increment);

#define FIELDNAMES_LENGTH 10
#define STACKVALUEILTYPE pInt64
#define	STACKVALUETYPE int64_t

#define MAX_RECURSIVE_INLINING_DEPTH 4

typedef std::map<const void *,OMR::JitBuilder::IlValue *> BlockToReceiverMap;

class SOMppMethod: public OMR::JitBuilder::MethodBuilder {
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
		ARRAY_DO,
		ARRAY_DOINDEXES,
		ARRAY_NEW,
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
		BOOLEAN_AND_NOBLOCK,
		BOOLEAN_OR,
		BOOLEAN_OR_NOBLOCK,
		BOOLEAN_NOT,
		OSR_TO_GENERIC_SEND,
		NOT_RECOGNIZED
	};
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

	static OMR::JitBuilder::IlValue *add(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *param1, OMR::JitBuilder::IlValue *param2);
	static OMR::JitBuilder::IlValue *sub(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *param1, OMR::JitBuilder::IlValue *param2);
	static OMR::JitBuilder::IlValue *mul(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *param1, OMR::JitBuilder::IlValue *param2);
	static OMR::JitBuilder::IlValue *div(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *param1, OMR::JitBuilder::IlValue *param2);
	static OMR::JitBuilder::IlValue *percent(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *param1, OMR::JitBuilder::IlValue *param2);
	static OMR::JitBuilder::IlValue *andVals(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *param1, OMR::JitBuilder::IlValue *param2);
	static OMR::JitBuilder::IlValue *lessThan(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *param1, OMR::JitBuilder::IlValue *param2);
	static OMR::JitBuilder::IlValue *greaterThan(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *param1, OMR::JitBuilder::IlValue *param2);
	static OMR::JitBuilder::IlValue *equalTo(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *param1, OMR::JitBuilder::IlValue *param2);
	static void forLoopUp(OMR::JitBuilder::BytecodeBuilder *builder, const char *index, OMR::JitBuilder::IlBuilder **loop, OMR::JitBuilder::IlValue *start, OMR::JitBuilder::IlValue *end, OMR::JitBuilder::IlValue *increment);
	static void forLoopDown(OMR::JitBuilder::BytecodeBuilder *builder, const char *index, OMR::JitBuilder::IlBuilder **loop, OMR::JitBuilder::IlValue *start, OMR::JitBuilder::IlValue *end, OMR::JitBuilder::IlValue *increment);
private:
	VMMethod *method;
	std::stack<VMMethod *> blockMethods;
	BlockToReceiverMap blockToReceiverMap;
	VMMethod *forLoopBlock;
	VMMethod *whileLoopConditionBlock;
	VMMethod *whileLoopCodeBlock;
	int32_t stackTopForErrorHandling[MAX_RECURSIVE_INLINING_DEPTH + 1];
	VMMethod *inlinedMethods[MAX_RECURSIVE_INLINING_DEPTH + 1];
	long inlinedBytecodeIndecies[MAX_RECURSIVE_INLINING_DEPTH + 1];
	int32_t extraStackDepthRequired;
	/* Should inlining be attempted */
	bool doInlining;
	bool doLoopInlining;
	/* Provide a name for the method */
	char methodName[64];

	void defineFunctions();
	void defineStructures(OMR::JitBuilder::TypeDictionary *types);
	void defineLocals();
	void defineParameters();

	void defineVMFrameStructure(OMR::JitBuilder::TypeDictionary *types);
	void defineVMObjectStructure(OMR::JitBuilder::TypeDictionary *types);

	void justReturn(OMR::JitBuilder::IlBuilder *from);

	OMR::JitBuilder::BytecodeBuilder **createBytecodesBuilder(VMMethod *vmMethod);
	int64_t calculateBytecodeIndexForJump(VMMethod *vmMethod, long bytecodeIndex);
	bool generateILForBytecodes(VMMethod *vmMethod, OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable);

	void doDup(OMR::JitBuilder::BytecodeBuilder *builder);
	void doPushLocal(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex);
	void doPushArgument(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex);
	void doPushField(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex);
	void doPushBlock(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex);
	void doPushConstant(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex);
	void doPushGlobal(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex);
	void doPop(OMR::JitBuilder::BytecodeBuilder *builder);
	void doPopLocal(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex);
	void doPopArgument(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex);
	void doPopField(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex);
	void doSend(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex, OMR::JitBuilder::BytecodeBuilder *fallThrough);
	void doSuperSend(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex);
	void doReturnLocal(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex);
	void doReturnNonLocal(OMR::JitBuilder::BytecodeBuilder *builder, long bytecodeIndex);
	void doJumpIfFalse(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex);
	void doJumpIfTrue(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex);
	void doJump(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **bytecodeBuilderTable, long bytecodeIndex);

	const char *getContext(OMR::JitBuilder::IlBuilder *builder, uint8_t level);
	OMR::JitBuilder::IlValue *getOuterContext(OMR::JitBuilder::IlBuilder *builder);
	OMR::JitBuilder::IlValue *getSelf(OMR::JitBuilder::IlBuilder *builder);
	void pushField(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *object, uint8_t fieldIndex);
	void pushConstant(OMR::JitBuilder::BytecodeBuilder *builder, VMMethod *vmMethod, uint8_t valueOffset);
	void pushValueFromArray(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *array, uint8_t arrayIndex);
	void pushGlobal(OMR::JitBuilder::BytecodeBuilder *builder, VMSymbol* globalName);
	void popField(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *object, uint8_t fieldIndex);
	void popValueToArray(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *array, uint8_t arrayIndex);
	OMR::JitBuilder::IlValue *getLocalArrayForLevel(OMR::JitBuilder::BytecodeBuilder *builder, VMMethod *vmMethod, long bytecodeIndex, int32_t recursiveLevel);
	OMR::JitBuilder::IlValue *getArgumentArrayForLevel(OMR::JitBuilder::BytecodeBuilder *builder, VMMethod *vmMethod, long bytecodeIndex, int32_t recursiveLevel);

	SOMppMethod::INLINE_STATUS doInlineIfPossible(OMR::JitBuilder::BytecodeBuilder **builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **merge, VMSymbol *signature, long bytecodeIndex);
	SOMppMethod::INLINE_STATUS generateRecognizedMethod(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **merge, SOMppMethod::RECOGNIZED_METHOD_INDEX recognizedMethodIndex, VMClass *receiverFromCache, long bytecodeIndex, int32_t recursiveLevel);
	void generateGenericMethod(OMR::JitBuilder::BytecodeBuilder **builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **merge, VMInvokable *invokable, VMClass *receiverClass, VMSymbol *signature, long bytecodeIndex, int32_t recursiveLevel = 0);
	void generateGenericMethodBody(OMR::JitBuilder::BytecodeBuilder **builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **merge, VMMethod *methodToInline, OMR::JitBuilder::IlValue *receiver, long bytecodeIndex, int32_t recursiveLevel = 0);
	void createBuildersForInlineSends(OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **merge, long bytecodeIndex);
	bool methodIsInlineable(VMMethod *vmMethod, int32_t recursiveLevel);

	SOMppMethod::INLINE_STATUS generateInlineForObjectNotEqual(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend,VMClass *receiverClass, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForObjectEqual(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, VMClass *receiverClass, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForObjectValue(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, VMClass *receiverClass, int32_t recursiveLevel);

	SOMppMethod::INLINE_STATUS generateInlineForGenericIsNil(OMR::JitBuilder::BytecodeBuilder *builder);
	SOMppMethod::INLINE_STATUS generateInlineForGenericNotNil(OMR::JitBuilder::BytecodeBuilder *builder);

	SOMppMethod::INLINE_STATUS generateInlineForIntegerMath(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, MathFuncType mathFunction, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForIntegerBoolean(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, BooleanFuncType booleanFunction, OMR::JitBuilder::IlValue *thenPathValue, OMR::JitBuilder::IlValue *elsePathValue, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForIntegerNegated(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForIntegerMax(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForIntegerAbs(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, long bytecodeIndex, int32_t recursiveLevel);

	void generateInlineForIntegerLoop(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, ForLoopFuncType loopFunction, OMR::JitBuilder::IlValue *start, OMR::JitBuilder::IlValue *end, OMR::JitBuilder::IlValue *increment, OMR::JitBuilder::IlValue *block, VMMethod *blockToInline, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForIntegerToDo(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForIntegerToByDo(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForIntegerDownToDo(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForIntegerDownToByDo(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel);

	SOMppMethod::INLINE_STATUS generateInlineForArrayAt(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForArrayAtPut(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForArrayLength(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForArrayDo(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForArrayDoIndexes(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForArrayNew(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel);

	SOMppMethod::INLINE_STATUS generateInlineForDoubleMath(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, MathFuncType mathFunction, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForDoubleBoolean(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, BooleanFuncType booleanFunction, OMR::JitBuilder::IlValue *thenPathValue, OMR::JitBuilder::IlValue *elsePathValue, int32_t recursiveLevel);

	void generateForWhileLoop(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel, OMR::JitBuilder::IlValue *condition);
	SOMppMethod::INLINE_STATUS generateInlineForWhileTrue(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForWhileFalse(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel);

	SOMppMethod::INLINE_STATUS generateInlineForBooleanAnd(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForBooleanAndNoBlock(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForBooleanOr(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForBooleanOrNoBlock(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel);
	SOMppMethod::INLINE_STATUS generateInlineForBooleanNot(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::BytecodeBuilder **mergeSend, long bytecodeIndex, int32_t recursiveLevel);

	SOMppMethod::INLINE_STATUS generateInlineOSRToGenericSend(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, long bytecodeIndex, int32_t recursiveLevel);

	void verifyIntegerArg(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::IlValue *value, int32_t recursiveLevel);
	void verifyDoubleArg(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::IlValue *object, OMR::JitBuilder::IlValue *objectClass, int32_t recursiveLevel);
	void verifyArg(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::IlValue *object, OMR::JitBuilder::IlValue *type, int32_t recursiveLevel);
	void verifyArg2(OMR::JitBuilder::BytecodeBuilder **builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::IlValue *object, OMR::JitBuilder::IlValue *type, VMMethod *methodToInline, VMSymbol* signature, long bytecodeIndex, int32_t recursiveLevel);
	void verifyBooleanArg(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::IlValue *object, int32_t recursiveLevel);
	OMR::JitBuilder::IlValue *getIntegerValue(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *object);
	OMR::JitBuilder::IlValue *newIntegerObjectForValue(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::BytecodeBuilder **genericSend, OMR::JitBuilder::IlValue *value, int32_t recursiveLevel);
	OMR::JitBuilder::IlValue *getDoubleValue(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *object);
	OMR::JitBuilder::IlValue *getDoubleValueFromDoubleOrInteger(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *object, OMR::JitBuilder::IlValue *objectClass);
	OMR::JitBuilder::IlValue *getIndexableFieldSlot(OMR::JitBuilder::BytecodeBuilder *builder, OMR::JitBuilder::IlValue *array, OMR::JitBuilder::IlValue *index);

	SOMppMethod::RECOGNIZED_METHOD_INDEX getRecognizedMethodIndex(VMMethod *sendingMethod, VMClass *receiverFromCache, VMClass *invokableClass, const char *signatureChars, int32_t recursiveLevel, bool doBlockInliningChecks = true);
};

#endif // !defined(SOMPPMETHOD_INCL)
