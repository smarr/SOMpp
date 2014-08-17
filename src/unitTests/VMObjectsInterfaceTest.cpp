/*
 * VMObjectsInterfaceTest.cpp
 *
 *  Created on: 02.03.2011
 *      Author: christian
 */

#include "VMObjectsInterfaceTest.h"

#include "WalkObjectsTest.h"
#include "vmobjects/VMSymbol.h"
#include "vmobjects/VMClass.h"
#include "vmobjects/VMDouble.h"
#include "vmobjects/VMBigInteger.h"
#include "vmobjects/VMInteger.h"
#include "vmobjects/VMArray.h"
#include "vmobjects/VMMethod.h"
#include "vmobjects/VMBlock.h"
#include "vmobjects/VMPrimitive.h"
#include "vmobjects/VMFrame.h"
#include "vmobjects/VMEvaluationPrimitive.h"

void VMObjectsInterfaceTest::setUp() {
    pObject = new (GetUniverse()->GetHeap()) VMObject();
    pInteger = GetUniverse()->NewInteger(42);
    pDouble = GetUniverse()->NewDouble(123.4);
    pString = GetUniverse()->NewString("foobar");
    pSymbol = GetUniverse()->NewSymbol("foobar");
    pArray = GetUniverse()->NewArray(0);
    pArray3 = GetUniverse()->NewArray(3);
    pMethod = GetUniverse()->NewMethod(pSymbol, 0, 0);
    pBlock = GetUniverse()->NewBlock(pMethod,
    GetUniverse()->GetInterpreter()->GetFrame(),
    pMethod->GetNumberOfArguments());
    pPrimitive = VMPrimitive::GetEmptyPrimitive(pSymbol);
    pBigInteger = GetUniverse()->NewBigInteger(0xdeadbeef);
    pClass = GetUniverse()->NewClass(integerClass);
    pFrame = GetUniverse()->GetInterpreter()->GetFrame();
    pEvaluationPrimitive = new (GetUniverse()->GetHeap()) VMEvaluationPrimitive(1);
}

void VMObjectsInterfaceTest::tearDown() {

}

void testObjectSizeHelper(VMOBJECT_PTR obj, StdString msg, size_t expectedSize) {
    CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, expectedSize, obj->GetObjectSize());
    obj->SetObjectSize(7654);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(msg + StdString(" (modified)"), (size_t)7654,
    obj->GetObjectSize());
    obj->SetObjectSize(expectedSize);
}

void VMObjectsInterfaceTest::testGetSetObjectSize() {
    testObjectSizeHelper(pObject, "plain object size", 24);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("integer size", (size_t) 12,
            pInteger->GetObjectSize());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("double size", (size_t) 16,
            pDouble->GetObjectSize());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("string size", (size_t) 20,
            pString->GetObjectSize());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("symbol size", (size_t) 56,
            pSymbol->GetObjectSize());
    //testObjectSizeHelper(pString, "string size", 12);
    //testObjectSizeHelper(pSymbol, "symbol size", 12);
    testObjectSizeHelper(pArray, "array size", 24);
    testObjectSizeHelper(pArray3, "array(3) size", 36);
    testObjectSizeHelper(pMethod, "method size", 60);
    testObjectSizeHelper(pBlock, "block size", 32);
    testObjectSizeHelper(pPrimitive, "primitive size", 40);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("big integer size", (size_t) 16,
            pBigInteger->GetObjectSize());
    testObjectSizeHelper(pClass, "class size", 44);
    testObjectSizeHelper(pFrame, "frame size", 64);
    testObjectSizeHelper(pEvaluationPrimitive, "evaluation primitive size", 44);
}

void testGCFieldHelper(AbstractVMObject* obj, StdString name) {
    size_t oldval = obj->GetGCField();
    obj->SetGCField((size_t) 0xaffe);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
            StdString("setting GCField failed for ") + name, (size_t) 0xaffe,
            obj->GetGCField());
    obj->SetGCField(oldval);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
            StdString("setting GCField failed for ") + name, oldval,
            obj->GetGCField());
}

void VMObjectsInterfaceTest::testGetSetGCField() {
    testGCFieldHelper(pObject, "plain object");
    testGCFieldHelper(pInteger, "integer");
    testGCFieldHelper(pDouble, "double");
    testGCFieldHelper(pString, "string");
    testGCFieldHelper(pSymbol, "symbol");
    testGCFieldHelper(pArray, "array");
    testGCFieldHelper(pArray3, "array(3)");
    testGCFieldHelper(pMethod, "method");
    testGCFieldHelper(pBlock, "block");
    testGCFieldHelper(pPrimitive, "primitive");
    testGCFieldHelper(pBigInteger, "big integer");
    testGCFieldHelper(pClass, "class");
    testGCFieldHelper(pFrame, "frame");
    testGCFieldHelper(pEvaluationPrimitive, "evaluation primitive");
}

void testNumberOfFieldsHelper(StdString name, long expectedNumberOfFields,
VMOBJECT_PTR obj) {
    //only decrease the size, otherwise we might access uninitialized memory and crash
    long targetSize = (expectedNumberOfFields > 0) ? expectedNumberOfFields
    - 1 : 0;
    obj->SetNumberOfFields(targetSize);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(name + StdString(
            " number of fields (modified) wrong!!!"), targetSize,
    obj->GetNumberOfFields());
    obj->SetNumberOfFields(expectedNumberOfFields);
    //ensure we got our original size back
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
    name + StdString(" number of fields wrong!!!"),
    expectedNumberOfFields, obj->GetNumberOfFields());
}

void VMObjectsInterfaceTest::testGetNumberOfFields() {
    //when setting the number of fields, all fields are nulled -> so use cloned objects instead
    testNumberOfFieldsHelper("plain object", 1, pObject->Clone());
    //Integer and doubles have no fields anymore
    //testNumberOfFieldsHelper("integer", 1, pInteger->Clone());
    //testNumberOfFieldsHelper("double", 1, pDouble->Clone());
    //testNumberOfFieldsHelper("string", 1, pString->Clone());
    //testNumberOfFieldsHelper("symbol", 1, pSymbol->Clone());
    testNumberOfFieldsHelper("array", 1, pArray->Clone());
    testNumberOfFieldsHelper("array(3)", 1, pArray3->Clone());
    testNumberOfFieldsHelper("method", 8, pMethod->Clone());
    testNumberOfFieldsHelper("block", 3, pBlock->Clone());
    testNumberOfFieldsHelper("primitive", 5, pPrimitive->Clone());
    //testNumberOfFieldsHelper("big integer", 1, pBigInteger->Clone());
    testNumberOfFieldsHelper("class", 6, pClass->Clone());
    testNumberOfFieldsHelper("frame", 7, pFrame->Clone());
    testNumberOfFieldsHelper("evaluation primitive", 5,
            pEvaluationPrimitive->Clone());
}

void VMObjectsInterfaceTest::testGetHash() {
    CPPUNIT_ASSERT_EQUAL_MESSAGE("plain object hash wrong", (size_t) pObject,
            pObject->GetHash());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("integer hash wrong", (size_t) pInteger,
            pInteger->GetHash());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("double hash wrong", (size_t) pDouble,
            pDouble->GetHash());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("string hash wrong", (size_t) pString,
            pString->GetHash());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("symbol hash wrong", (size_t) pSymbol,
            pSymbol->GetHash());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("array hash wrong", (size_t) pArray,
            pArray->GetHash());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("array(3) hash wrong", (size_t) pArray3,
            pArray3->GetHash());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("method hash wrong", (size_t) pMethod,
            pMethod->GetHash());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("block hash wrong", (size_t) pBlock,
            pBlock->GetHash());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("primitive hash wrong", (size_t) pPrimitive,
            pPrimitive->GetHash());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("big integer hash wrong", (size_t) pBigInteger,
            pBigInteger->GetHash());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class hash wrong", (size_t) pClass,
            pClass->GetHash());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("frame hash wrong", (size_t) pFrame,
            pFrame->GetHash());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("evaluation primitive hash wrong",
            (size_t) pEvaluationPrimitive, pEvaluationPrimitive->GetHash());

}

pVMClass getGlobalClass(const char* name) {
    return (pVMClass) GetUniverse()->GetGlobal(GetUniverse()->SymbolForChars(name));
}
