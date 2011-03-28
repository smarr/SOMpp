/*
 * CloneObjectsTest.cpp
 *
 *  Created on: 21.01.2011
 *      Author: christian
 */

#include "CloneObjectsTest.h"

#define private public
#define protected public

#include "vmobjects/VMObject.h"
#include "vmobjects/VMInteger.h"
#include "vmobjects/VMDouble.h"
#include "vmobjects/VMString.h"
#include "vmobjects/VMBigInteger.h"
#include "vmobjects/VMArray.h"
#include "vmobjects/VMMethod.h"
#include "vmobjects/VMBlock.h"
#include "vmobjects/VMSymbol.h"
#include "vmobjects/VMClass.h"
#include "vmobjects/VMPrimitive.h"
#include "vmobjects/VMEvaluationPrimitive.h"

void CloneObjectsTest::testCloneObject() {
	pVMObject orig = new (_UNIVERSE->GetHeap()) VMObject();
	pVMObject clone = orig->Clone();
	CPPUNIT_ASSERT((int)orig != (int)clone);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->GetClass(),
			clone->GetClass());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->GetObjectSize(),
			clone->GetObjectSize());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!",
			orig->GetNumberOfFields(), clone->GetNumberOfFields());
}

void CloneObjectsTest::testCloneInteger() {
	pVMInteger orig = _UNIVERSE->NewInteger(42);
	pVMInteger clone = orig->Clone();

	CPPUNIT_ASSERT((int)orig != (int)clone);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->GetClass(), clone->GetClass());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("integer value differs!!", orig->embeddedInteger, clone->embeddedInteger);
}

void CloneObjectsTest::testCloneDouble() {
	pVMDouble orig = _UNIVERSE->NewDouble(123.4);
	pVMDouble clone = orig->Clone();

	CPPUNIT_ASSERT((int)orig != (int)clone);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->GetClass(), clone->GetClass());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("double value differs!!", orig->embeddedDouble, clone->embeddedDouble);
}

void CloneObjectsTest::testCloneString() {
	pVMString orig = _UNIVERSE->NewString("foobar");
	pVMString clone = orig->Clone();

	CPPUNIT_ASSERT((int)orig != (int)clone);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->GetClass(),
			clone->GetClass());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->GetObjectSize(),
			clone->GetObjectSize());
	//CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!", orig->numberOfFields, clone->numberOfFields);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("string differs!!!", orig->GetStdString(), clone->GetStdString());
	//CPPUNIT_ASSERT_MESSAGE("internal string was not copied", (int)orig->chars != (int)clone->chars);
	orig->chars[0] = 'm';
	CPPUNIT_ASSERT_MESSAGE("string differs!!!", orig->GetStdString() != clone->GetStdString());

}

void CloneObjectsTest::testCloneSymbol() {
	pVMSymbol orig = _UNIVERSE->NewSymbol("foobar");
	pVMSymbol clone = orig->Clone();

	CPPUNIT_ASSERT((int)orig != (int)clone);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->GetClass(),
			clone->GetClass());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->GetObjectSize(),
			clone->GetObjectSize());
	//CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!", orig->numberOfFields, clone->numberOfFields);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("string differs!!!", orig->GetPlainString(), clone->GetPlainString());
}

void CloneObjectsTest::testCloneBigInteger() {
	pVMBigInteger orig = _UNIVERSE->NewBigInteger(0xdeadbeef);
	pVMBigInteger clone = orig->Clone();

	CPPUNIT_ASSERT((int)orig != (int)clone);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->GetClass(), clone->GetClass());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("bigint value differs!!", orig->embeddedInteger, clone->embeddedInteger);
}

void CloneObjectsTest::testCloneArray() {
	pVMArray orig = _UNIVERSE->NewArray(3);
	(*orig)[0] = _UNIVERSE->NewString("foobar42");
	(*orig)[1] = _UNIVERSE->NewString("foobar43");
	(*orig)[2] = _UNIVERSE->NewString("foobar44");
	pVMArray clone = orig->Clone();

	CPPUNIT_ASSERT((int)orig != (int)clone);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->clazz, clone->clazz);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->objectSize, clone->objectSize);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!", orig->numberOfFields, clone->numberOfFields);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!", orig->GetNumberOfIndexableFields(), clone->GetNumberOfIndexableFields());

	for (int i = 0; i < 3; ++i)
		CPPUNIT_ASSERT((*orig)[i] == (*clone)[i]);
}

void CloneObjectsTest::testCloneBlock() {
	pVMSymbol methodSymbol = _UNIVERSE->NewSymbol("someMethod");
	pVMMethod method = _UNIVERSE->NewMethod(methodSymbol, 0, 0);
	pVMBlock orig = _UNIVERSE->NewBlock(method,
			_UNIVERSE->GetInterpreter()->GetFrame(),
			method->GetNumberOfArguments());
	pVMBlock clone = orig->Clone();

	CPPUNIT_ASSERT((int)orig != (int)clone);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->clazz, clone->clazz);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->objectSize, clone->objectSize);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!", orig->numberOfFields, clone->numberOfFields);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("blockMethod differs!!", orig->blockMethod, clone->blockMethod);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("context differs!!", orig->context, clone->context);
}
void CloneObjectsTest::testClonePrimitive() {
	pVMSymbol primitiveSymbol = _UNIVERSE->NewSymbol("myPrimitive");
	pVMPrimitive orig = VMPrimitive::GetEmptyPrimitive(primitiveSymbol);
	pVMPrimitive clone = orig->Clone();
	CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->clazz, clone->clazz);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->objectSize, clone->objectSize);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!", orig->numberOfFields, clone->numberOfFields);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("signature differs!!", orig->signature, clone->signature);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("holder differs!!", orig->holder, clone->holder);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("empty differs!!", orig->empty, clone->empty);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("routine differs!!", orig->routine, clone->routine);
}

void CloneObjectsTest::testCloneEvaluationPrimitive() {
	pVMEvaluationPrimitive orig = new (_UNIVERSE->GetHeap()) VMEvaluationPrimitive(1);
	pVMEvaluationPrimitive clone = orig->Clone();

	CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->clazz, clone->clazz);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->objectSize, clone->objectSize);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!", orig->numberOfFields, clone->numberOfFields);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("signature differs!!", orig->signature, clone->signature);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("holder differs!!", orig->holder, clone->holder);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("empty differs!!", orig->empty, clone->empty);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("routine differs!!", orig->routine, clone->routine);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfArguments differs!!", orig->numberOfArguments, clone->numberOfArguments);
}

void CloneObjectsTest::testCloneFrame() {
	pVMSymbol methodSymbol = _UNIVERSE->NewSymbol("frameMethod");
	pVMMethod method = _UNIVERSE->NewMethod(methodSymbol, 0, 0);
	pVMFrame orig = _UNIVERSE->NewFrame(NULL, method);
	pVMInteger dummyArg = _UNIVERSE->NewInteger(1111);
	orig->SetArgument(0, 0, dummyArg);
	pVMFrame clone = orig->Clone();

	CPPUNIT_ASSERT((int)orig != (int)clone);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->clazz, clone->clazz);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->objectSize, clone->objectSize);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!", orig->numberOfFields, clone->numberOfFields);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!", orig->GetNumberOfIndexableFields(), clone->GetNumberOfIndexableFields());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("GetPreviousFrame differs!!", orig->GetPreviousFrame(), clone->GetPreviousFrame());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("GetContext differs!!", orig->GetContext(), clone->GetContext());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOGetMethodfFields differs!!", orig->GetMethod(), clone->GetMethod());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("GetStackPointer differs!!", orig->GetStackPointer(), clone->GetStackPointer());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("bytecodeIndex differs!!", orig->bytecodeIndex, clone->bytecodeIndex);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("localOffset differs!!", orig->localOffset, clone->localOffset);
	//CPPUNIT_ASSERT_EQUAL_MESSAGE("GetArgument differs!!", orig->GetArgument(0,0), clone->GetArgument(0,0));
}

void CloneObjectsTest::testCloneMethod() {
	pVMSymbol methodSymbol = _UNIVERSE->NewSymbol("myMethod");
	pVMMethod orig = _UNIVERSE->NewMethod(methodSymbol, 0, 0);
	pVMMethod clone = orig->Clone();

	CPPUNIT_ASSERT((int)orig != (int)clone);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->clazz, clone->clazz);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->objectSize, clone->objectSize);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!", orig->numberOfFields, clone->numberOfFields);

	CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfLocals differs!!", orig->numberOfLocals, clone->numberOfLocals);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("bcLength differs!!", orig->bcLength, clone->bcLength);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("maximumNumberOfStackElements differs!!", orig->maximumNumberOfStackElements, clone->maximumNumberOfStackElements);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfArguments differs!!", orig->numberOfArguments, clone->numberOfArguments);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfConstants differs!!", orig->numberOfConstants, clone->numberOfConstants);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("GetHolder() differs!!", orig->GetHolder(), clone->GetHolder());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("GetSignature() differs!!", orig->GetSignature(), clone->GetSignature());
}

void CloneObjectsTest::testCloneClass() {
	pVMClass orig = _UNIVERSE->NewClass(integerClass);
	orig->SetName(_UNIVERSE->NewSymbol("MyClass"));
	orig->SetSuperClass(doubleClass);
	orig->SetInstanceFields(_UNIVERSE->NewArray(2));
	orig->SetInstanceInvokables(_UNIVERSE->NewArray(4));
	pVMClass clone = orig->Clone();

	CPPUNIT_ASSERT((int)orig != (int)clone);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->clazz, clone->clazz);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->objectSize, clone->objectSize);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!", orig->numberOfFields, clone->numberOfFields);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("superClass differs!!", orig->superClass, clone->superClass);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("name differs!!", orig->name, clone->name);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("instanceFields differs!!", orig->instanceFields, clone->instanceFields);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("instanceInvokables differs!!", orig->instanceInvokables, clone->instanceInvokables);
}
