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
	pObject = new (_UNIVERSE->GetHeap()) VMObject();
	pInteger = _UNIVERSE->NewInteger(42);
	pDouble = _UNIVERSE->NewDouble(123.4);
	pString = _UNIVERSE->NewString("foobar");
	pSymbol = _UNIVERSE->NewSymbol("foobar");
	pArray = _UNIVERSE->NewArray(0);
	pArray3 = _UNIVERSE->NewArray(3);
	pMethod = _UNIVERSE->NewMethod(pSymbol, 0, 0);
	pBlock = _UNIVERSE->NewBlock(pMethod,
			_UNIVERSE->GetInterpreter()->GetFrame(),
			pMethod->GetNumberOfArguments());
	pPrimitive = VMPrimitive::GetEmptyPrimitive(pSymbol);
	pBigInteger = _UNIVERSE->NewBigInteger(0xdeadbeef);
	pClass = _UNIVERSE->NewClass(integerClass);
	pFrame = _UNIVERSE->GetInterpreter()->GetFrame();
	pEvaluationPrimitive = new (_UNIVERSE->GetHeap()) VMEvaluationPrimitive(1);
}

void VMObjectsInterfaceTest::tearDown() {

}

void testObjectSizeHelper(pVMObject obj, StdString msg, int32_t expectedSize) {
	CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, expectedSize, obj->GetObjectSize());
	obj->SetObjectSize(7654);
	CPPUNIT_ASSERT_EQUAL_MESSAGE(msg + StdString(" (modified)"), 7654,
			obj->GetObjectSize());
	obj->SetObjectSize(expectedSize);
}

void VMObjectsInterfaceTest::testGetSetObjectSize() {
	testObjectSizeHelper(pObject, "plain object size", 24);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("integer size", 12, pInteger->GetObjectSize());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("double size", 16, pDouble->GetObjectSize());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("string size", 20, pString->GetObjectSize());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("symbol size", 28, pSymbol->GetObjectSize());
	//testObjectSizeHelper(pString, "string size", 12);
	//testObjectSizeHelper(pSymbol, "symbol size", 12);
	testObjectSizeHelper(pArray, "array size", 24);
	testObjectSizeHelper(pArray3, "array(3) size", 36);
	testObjectSizeHelper(pMethod, "method size", 52);
	testObjectSizeHelper(pBlock, "block size", 32);
	testObjectSizeHelper(pPrimitive, "primitive size", 40);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("big integer size", 16,
			pBigInteger->GetObjectSize());
	testObjectSizeHelper(pClass, "class size", 44);
	testObjectSizeHelper(pFrame, "frame size", 60);
	testObjectSizeHelper(pEvaluationPrimitive, "evaluation primitive size", 44);
}

void testGCFieldHelper(AbstractVMObject* obj, StdString name) {
	int32_t oldval = obj->GetGCField();
	obj->SetGCField(0xaffe);
	CPPUNIT_ASSERT_EQUAL_MESSAGE(StdString("setting GCField failed for ")
			+ name, 0xaffe, obj->GetGCField());
	obj->SetGCField(oldval);
	CPPUNIT_ASSERT_EQUAL_MESSAGE(StdString("setting GCField failed for ")
			+ name, oldval, obj->GetGCField());
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

void testNumberOfFieldsHelper(StdString name, int32_t expectedNumberOfFields,
		pVMObject obj) {
	//only decrease the size, otherwise we might access uninitialized memory and crash
	int32_t targetSize = (expectedNumberOfFields > 0) ? expectedNumberOfFields
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

void testGetClassFieldHelper(pVMObject obj, StdString name) {
	pVMSymbol sym = _UNIVERSE->SymbolFor("class");
	int fieldNo = obj->GetFieldIndex(sym);
	CPPUNIT_ASSERT_EQUAL_MESSAGE(name + StdString(
			" does not have 'class' as first field"), 0, fieldNo);
	CPPUNIT_ASSERT_EQUAL_MESSAGE(name + StdString(
			"does not support 'class' field"), (pVMClass)obj->GetField(fieldNo),
			obj->GetClass());
}

void VMObjectsInterfaceTest::testGetClassField() {
	//Don't touch CLASS of plain Object -> Not set to anything -> Danger
	testGetClassFieldHelper(pInteger, "integer");
	testGetClassFieldHelper(pDouble, "double");
	testGetClassFieldHelper(pString, "string");
	testGetClassFieldHelper(pSymbol, "symbol");
	testGetClassFieldHelper(pArray, "array");
	testGetClassFieldHelper(pArray3, "array(3)");
	testGetClassFieldHelper(pMethod, "method");
	testGetClassFieldHelper(pBlock, "block");
	testGetClassFieldHelper(pPrimitive, "primitive");
	testGetClassFieldHelper(pBigInteger, "big integer");
	testGetClassFieldHelper(pClass, "class");
	testGetClassFieldHelper(pFrame, "frame");
	testGetClassFieldHelper(pEvaluationPrimitive, "evaluation primitive");

}

void testGetSetFieldHelper(pVMObject obj, StdString name) {
	for (int32_t i = 0; i <= obj->GetNumberOfFields(); i++) {
		AbstractVMObject* oldVal = obj->GetField(i);
		//set field to another value and check if it has changed
		AbstractVMObject* otherObject = oldVal == integerClass ? stringClass
				: integerClass;
		obj->SetField(i, otherObject);
		CPPUNIT_ASSERT_EQUAL_MESSAGE(
				"getField doesn't return what was set before", otherObject,
#ifdef USE_TAGGING
				obj->GetField(i).GetPointer());
#else
				obj->GetField(i));
#endif
		//now reset the field and check again
		obj->SetField(i, oldVal);
		CPPUNIT_ASSERT_EQUAL_MESSAGE(
				"getField doesn't return what was set before", oldVal,
#ifdef USE_TAGGING
				obj->GetField(i).GetPointer());
#else
				obj->GetField(i));
#endif
	}
}

void testFieldNameHelper(pVMObject obj, StdString objectName, int32_t index,
		StdString expectedName) {
	std::stringstream message;
	message << objectName << " field " << index;
	CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), expectedName, StdString(
			obj->GetFieldName(index)->GetChars()));
}

void VMObjectsInterfaceTest::testGetFieldName() {
	//	testFieldNameHelper(pObject, "plain object", 0, "class");
}

void VMObjectsInterfaceTest::testGetSetField() {
	testGetSetFieldHelper(pObject, "plain object");
	//Integer and Doubles have no fields anymore
	//testGetSetFieldHelper(pInteger, "integer");
	//testGetSetFieldHelper(pDouble, "double");
	//testGetSetFieldHelper(pString, "string");
	//testGetSetFieldHelper(pSymbol, "symbol");
	testGetSetFieldHelper(pArray, "array");
	testGetSetFieldHelper(pArray3, "array(3)");
	testGetSetFieldHelper(pMethod, "method");
	testGetSetFieldHelper(pBlock, "block");
	testGetSetFieldHelper(pPrimitive, "primitive");
	//testGetSetFieldHelper(pBigInteger, "big integer");
	testGetSetFieldHelper(pClass, "class");
	testGetSetFieldHelper(pFrame, "frame");
	testGetSetFieldHelper(pEvaluationPrimitive, "evaluation primitive");
}

void VMObjectsInterfaceTest::testGetHash() {
	CPPUNIT_ASSERT_EQUAL_MESSAGE("plain object hash wrong", (int32_t) pObject,
			pObject->GetHash());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("integer hash wrong", (int32_t) pInteger,
			pInteger->GetHash());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("double hash wrong", (int32_t) pDouble,
			pDouble->GetHash());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("string hash wrong", (int32_t) pString,
			pString->GetHash());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("symbol hash wrong", (int32_t) pSymbol,
			pSymbol->GetHash());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("array hash wrong", (int32_t) pArray,
			pArray->GetHash());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("array(3) hash wrong", (int32_t) pArray3,
			pArray3->GetHash());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("method hash wrong", (int32_t) pMethod,
			pMethod->GetHash());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("block hash wrong", (int32_t) pBlock,
			pBlock->GetHash());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("primitive hash wrong", (int32_t) pPrimitive,
			pPrimitive->GetHash());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("big integer hash wrong",
			(int32_t) pBigInteger, pBigInteger->GetHash());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("class hash wrong", (int32_t) pClass,
			pClass->GetHash());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("frame hash wrong", (int32_t) pFrame,
			pFrame->GetHash());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("evaluation primitive hash wrong",
			(int32_t) pEvaluationPrimitive, pEvaluationPrimitive->GetHash());

}

pVMClass getGlobalClass(const char* name) {
	return (pVMClass) _UNIVERSE->GetGlobal(_UNIVERSE->SymbolForChars(name));
}

void testGetSetClassHelper(StdString name, pVMObject obj,
		pVMClass expectedClass, pVMClass otherClass) {
	//first check if it has the expected class
	CPPUNIT_ASSERT_EQUAL_MESSAGE(name + " not instance of "
			+ expectedClass->GetName()->GetChars(), expectedClass,
			obj->GetClass());
	//now change the class and check if it has changed
	obj->SetClass(otherClass);
	CPPUNIT_ASSERT_EQUAL_MESSAGE(name + " not instance of (after SetClass) "
			+ expectedClass->GetName()->GetChars(), otherClass, obj->GetClass());
	//now change back and see if everything is as expected again
	obj->SetClass(expectedClass);
	CPPUNIT_ASSERT_EQUAL_MESSAGE(name + " not instance of (after 2xSetClass) "
			+ expectedClass->GetName()->GetChars(), expectedClass,
			obj->GetClass());
}

void VMObjectsInterfaceTest::testGetSetClass() {
	//Don't touch GetClass of Object -> Not set to anything -> Danger
	//CPPUNIT_ASSERT_EQUAL_MESSAGE("plain object class wrong!!!", getGlobalClass("Metaclass"), pObject->GetClass());
	//There is no setter for Integer and Double classes anymore
	CPPUNIT_ASSERT_EQUAL_MESSAGE("integer not instance of Integer class",
			getGlobalClass("Integer"), pInteger->GetClass());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("double not instance of Double class",
			getGlobalClass("Double"), pDouble->GetClass());
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
			"big integer not instance of big integer class", getGlobalClass(
					"BigInteger"), pBigInteger->GetClass());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("string not instance of String class",
			getGlobalClass("String"), pString->GetClass());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("symbol not instance of Symbol class",
			getGlobalClass("Symbol"), pSymbol->GetClass());
	testGetSetClassHelper("array", pArray, getGlobalClass("Array"),
			getGlobalClass("String"));
	testGetSetClassHelper("method", pMethod, getGlobalClass("Method"),
			getGlobalClass("String"));
	testGetSetClassHelper("block", pBlock, getGlobalClass("Block1"),
			getGlobalClass("String"));
	testGetSetClassHelper("primitive", pPrimitive, getGlobalClass("Primitive"),
			getGlobalClass("String"));
	// pClass is created as Integer -> Integer should be correct
	testGetSetClassHelper("class", pClass, getGlobalClass("Integer"),
			getGlobalClass("String"));
	testGetSetClassHelper("frame", pFrame, getGlobalClass("Frame"),
			getGlobalClass("String"));
	testGetSetClassHelper("evaluation primitive", pEvaluationPrimitive,
			getGlobalClass("Primitive"), getGlobalClass("String"));
}

