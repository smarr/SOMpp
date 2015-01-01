/*
 * CloneObjectsTest.cpp
 *
 *  Created on: 21.01.2011
 *      Author: christian
 */

#include "CloneObjectsTest.h"

#define private public
#define protected public

#include "vmobjects/VMObjectBase.h"
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
    pVMObject orig = new (GetHeap<HEAP_CLS>()) VMObject();
    pVMObject clone = orig->Clone();
    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->GetClass(),
    clone->GetClass());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->GetObjectSize(),
    clone->GetObjectSize());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!",
    orig->GetNumberOfFields(), clone->GetNumberOfFields());
}

void CloneObjectsTest::testCloneInteger() {
    pVMInteger orig = GetUniverse()->NewInteger(42);
    pVMInteger clone = orig->Clone();

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->GetClass(), clone->GetClass());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("integer value differs!!", orig->embeddedInteger, clone->embeddedInteger);
}

void CloneObjectsTest::testCloneDouble() {
    VMDouble* orig = GetUniverse()->NewDouble(123.4);
    VMDouble* clone = orig->Clone();

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->GetClass(), clone->GetClass());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("double value differs!!", orig->embeddedDouble, clone->embeddedDouble);
}

void CloneObjectsTest::testCloneString() {
    pVMString orig = GetUniverse()->NewString("foobar");
    pVMString clone = orig->Clone();

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->GetClass(),
            clone->GetClass());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->GetObjectSize(),
            clone->GetObjectSize());
    //CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!", orig->numberOfFields, clone->numberOfFields);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("string differs!!!", orig->GetStdString(), clone->GetStdString());
    //CPPUNIT_ASSERT_MESSAGE("internal string was not copied", (intptr_t)orig->chars != (intptr_t)clone->chars);
    orig->chars[0] = 'm';
    CPPUNIT_ASSERT_MESSAGE("string differs!!!", orig->GetStdString() != clone->GetStdString());

}

void CloneObjectsTest::testCloneSymbol() {
    pVMSymbol orig = GetUniverse()->NewSymbol("foobar");
    pVMSymbol clone = orig->Clone();

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->GetClass(),
            clone->GetClass());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->GetObjectSize(),
            clone->GetObjectSize());
    //CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!", orig->numberOfFields, clone->numberOfFields);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("string differs!!!", orig->GetPlainString(), clone->GetPlainString());
}

void CloneObjectsTest::testCloneBigInteger() {
    VMBigInteger* orig = GetUniverse()->NewBigInteger(0xdeadbeef);
    VMBigInteger* clone = orig->Clone();

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->GetClass(), clone->GetClass());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("bigint value differs!!", orig->embeddedInteger, clone->embeddedInteger);
}

void CloneObjectsTest::testCloneArray() {
    VMArray* orig = GetUniverse()->NewArray(3);
    orig->SetIndexableField(0, GetUniverse()->NewString("foobar42"));
    orig->SetIndexableField(1, GetUniverse()->NewString("foobar43"));
    orig->SetIndexableField(2, GetUniverse()->NewString("foobar44"));
    VMArray* clone = orig->Clone();

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->clazz, clone->clazz);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->objectSize, clone->objectSize);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!", orig->numberOfFields, clone->numberOfFields);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!", orig->GetNumberOfIndexableFields(), clone->GetNumberOfIndexableFields());

    CPPUNIT_ASSERT_EQUAL_MESSAGE("field 0 differs", orig->GetIndexableField(0),
            clone->GetIndexableField(0));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("field 1 differs", orig->GetIndexableField(1),
            clone->GetIndexableField(1));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("field 2 differs", orig->GetIndexableField(2),
            clone->GetIndexableField(2));
}

void CloneObjectsTest::testCloneBlock() {
    pVMSymbol methodSymbol = GetUniverse()->NewSymbol("someMethod");
    pVMMethod method = GetUniverse()->NewMethod(methodSymbol, 0, 0);
    VMBlock* orig = GetUniverse()->NewBlock(method,
            GetUniverse()->GetInterpreter()->GetFrame(),
            method->GetNumberOfArguments());
    VMBlock* clone = orig->Clone();

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->clazz, clone->clazz);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->objectSize, clone->objectSize);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!", orig->numberOfFields, clone->numberOfFields);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("blockMethod differs!!", orig->blockMethod, clone->blockMethod);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("context differs!!", orig->context, clone->context);
}
void CloneObjectsTest::testClonePrimitive() {
    pVMSymbol primitiveSymbol = GetUniverse()->NewSymbol("myPrimitive");
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
    pVMEvaluationPrimitive orig = new (GetHeap<HEAP_CLS>()) VMEvaluationPrimitive(1);
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
    pVMSymbol methodSymbol = GetUniverse()->NewSymbol("frameMethod");
    pVMMethod method = GetUniverse()->NewMethod(methodSymbol, 0, 0);
    pVMFrame orig = GetUniverse()->NewFrame(nullptr, method);
    pVMFrame context = orig->Clone();
    orig->SetContext(context);
    pVMInteger dummyArg = GetUniverse()->NewInteger(1111);
    orig->SetArgument(0, 0, dummyArg);
    pVMFrame clone = orig->Clone();

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->clazz, clone->clazz);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->objectSize, clone->objectSize);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!", orig->numberOfFields, clone->numberOfFields);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("GetPreviousFrame differs!!", orig->GetPreviousFrame(), clone->GetPreviousFrame());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("GetContext differs!!", orig->GetContext(), clone->GetContext());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOGetMethodfFields differs!!", orig->GetMethod(), clone->GetMethod());
    //CPPUNIT_ASSERT_EQUAL_MESSAGE("GetStackPointer differs!!", orig->GetStackPointer(), clone->GetStackPointer());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("bytecodeIndex differs!!", orig->bytecodeIndex, clone->bytecodeIndex);
}

void CloneObjectsTest::testCloneMethod() {
    pVMSymbol methodSymbol = GetUniverse()->NewSymbol("myMethod");
    pVMMethod orig = GetUniverse()->NewMethod(methodSymbol, 0, 0);
    pVMMethod clone = orig->Clone();

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->clazz, clone->clazz);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->objectSize, clone->objectSize);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!", orig->numberOfFields, clone->numberOfFields);

    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfLocals differs!!",
            INT_VAL(orig->numberOfLocals),
            INT_VAL(clone->numberOfLocals));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("bcLength differs!!",
            INT_VAL(orig->bcLength),
            INT_VAL(clone->bcLength));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("maximumNumberOfStackElements differs!!",
            INT_VAL(orig->maximumNumberOfStackElements),
            INT_VAL(clone->maximumNumberOfStackElements));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfArguments differs!!",
            INT_VAL(orig->numberOfArguments),
            INT_VAL(clone->numberOfArguments));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfConstants differs!!",
            INT_VAL(orig->numberOfConstants),
            INT_VAL(clone->numberOfConstants));

    CPPUNIT_ASSERT_EQUAL_MESSAGE("GetHolder() differs!!", orig->GetHolder(), clone->GetHolder());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("GetSignature() differs!!", orig->GetSignature(), clone->GetSignature());
}

void CloneObjectsTest::testCloneClass() {
    pVMClass orig = GetUniverse()->NewClass(integerClass);
    orig->SetName(GetUniverse()->NewSymbol("MyClass"));
    orig->SetSuperClass(doubleClass);
    orig->SetInstanceFields(GetUniverse()->NewArray(2));
    orig->SetInstanceInvokables(GetUniverse()->NewArray(4));
    pVMClass clone = orig->Clone();

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->clazz, clone->clazz);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->objectSize, clone->objectSize);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!", orig->numberOfFields, clone->numberOfFields);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("superClass differs!!", orig->superClass, clone->superClass);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("name differs!!", orig->name, clone->name);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("instanceFields differs!!", orig->instanceFields, clone->instanceFields);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("instanceInvokables differs!!", orig->instanceInvokables, clone->instanceInvokables);
}
