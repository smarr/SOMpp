/*
 * CloneObjectsTest.cpp
 *
 *  Created on: 21.01.2011
 *      Author: christian
 */

#include "CloneObjectsTest.h"

#define private public
#define protected public

#include "vmobjects/ObjectFormats.h"
#include "vmobjects/VMObjectBase.h"
#include "vmobjects/VMObject.h"
#include "vmobjects/VMInteger.h"
#include "vmobjects/VMDouble.h"
#include "vmobjects/VMString.h"
#include "vmobjects/VMArray.h"
#include "vmobjects/VMMethod.h"
#include "vmobjects/VMBlock.h"
#include "vmobjects/VMSymbol.h"
#include "vmobjects/VMClass.h"
#include "vmobjects/VMPrimitive.h"
#include "vmobjects/VMEvaluationPrimitive.h"

void CloneObjectsTest::testCloneObject() {
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    VMObject* orig = new (page) VMObject();
    VMObject* clone = orig->Clone(page);
    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->GetClass(),
    clone->GetClass());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->GetObjectSize(),
    clone->GetObjectSize());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!",
    orig->GetNumberOfFields(), clone->GetNumberOfFields());
}

void CloneObjectsTest::testCloneInteger() {
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    VMInteger* orig = GetUniverse()->NewInteger(42, page);
    VMInteger* clone = orig->Clone(page);

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->GetClass(), clone->GetClass());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("integer value differs!!", orig->embeddedInteger, clone->embeddedInteger);
}

void CloneObjectsTest::testCloneDouble() {
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    VMDouble* orig = GetUniverse()->NewDouble(123.4, page);
    VMDouble* clone = orig->Clone(page);

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->GetClass(), clone->GetClass());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("double value differs!!", orig->embeddedDouble, clone->embeddedDouble);
}

void CloneObjectsTest::testCloneString() {
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    VMString* orig = GetUniverse()->NewString("foobar", page);
    VMString* clone = orig->Clone(page);

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
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    VMSymbol* orig = GetUniverse()->NewSymbol("foobar", page);
    VMSymbol* clone = orig->Clone(page);

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->GetClass(),
            clone->GetClass());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->GetObjectSize(),
            clone->GetObjectSize());
    //CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!", orig->numberOfFields, clone->numberOfFields);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("string differs!!!", orig->GetPlainString(), clone->GetPlainString());
}

void CloneObjectsTest::testCloneArray() {
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    VMArray* orig = GetUniverse()->NewArray(3, page);
    orig->SetIndexableField(0, GetUniverse()->NewString("foobar42", page));
    orig->SetIndexableField(1, GetUniverse()->NewString("foobar43", page));
    orig->SetIndexableField(2, GetUniverse()->NewString("foobar44", page));
    VMArray* clone = orig->Clone(page);

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
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    Interpreter interp(page);
    page->SetInterpreter(&interp);

    
    VMSymbol* methodSymbol = GetUniverse()->NewSymbol("someMethod", page);
    VMMethod* method = GetUniverse()->NewMethod(methodSymbol, 0, 0, page);
    VMBlock* orig = GetUniverse()->NewBlock(method,
            GetUniverse()->NewFrame(nullptr, method, page),
            method->GetNumberOfArguments(), page);
    VMBlock* clone = orig->Clone(page);

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->clazz, clone->clazz);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->objectSize, clone->objectSize);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!", orig->numberOfFields, clone->numberOfFields);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("blockMethod differs!!", orig->blockMethod, clone->blockMethod);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("context differs!!", orig->context, clone->context);
}
void CloneObjectsTest::testClonePrimitive() {
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    VMSymbol* primitiveSymbol = GetUniverse()->NewSymbol("myPrimitive", page);
    VMPrimitive* orig = VMPrimitive::GetEmptyPrimitive(primitiveSymbol, false, page);
    VMPrimitive* clone = orig->Clone(page);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->clazz, clone->clazz);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->objectSize, clone->objectSize);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!", orig->numberOfFields, clone->numberOfFields);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("signature differs!!", orig->signature, clone->signature);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("holder differs!!", orig->holder, clone->holder);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("empty differs!!", orig->empty, clone->empty);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("routine differs!!", orig->routine, clone->routine);
}

void CloneObjectsTest::testCloneEvaluationPrimitive() {
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    VMEvaluationPrimitive* orig = new (page) VMEvaluationPrimitive(1, page);
    VMEvaluationPrimitive* clone = orig->Clone(page);

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
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    Interpreter* interp = new Interpreter(page);
    page->SetInterpreter(interp);
    
    VMSymbol* methodSymbol = GetUniverse()->NewSymbol("frameMethod", page);
    VMMethod* method = GetUniverse()->NewMethod(methodSymbol, 0, 0, page);
    VMFrame* orig = GetUniverse()->NewFrame(nullptr, method, page);
    VMFrame* context = orig->Clone(page);
    orig->SetContext(context);
    VMInteger* dummyArg = GetUniverse()->NewInteger(1111, page);
    orig->SetArgument(0, 0, dummyArg);
    VMFrame* clone = orig->Clone(page);

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
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    VMSymbol* methodSymbol = GetUniverse()->NewSymbol("myMethod", page);
    VMMethod* orig = GetUniverse()->NewMethod(methodSymbol, 0, 0, page);
    VMMethod* clone = orig->Clone(page);

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->clazz, clone->clazz);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->objectSize, clone->objectSize);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!", orig->numberOfFields, clone->numberOfFields);

    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfLocals differs!!",
            INT_VAL(load_ptr(orig->numberOfLocals)),
            INT_VAL(load_ptr(clone->numberOfLocals)));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("bcLength differs!!",
            INT_VAL(load_ptr(orig->bcLength)),
            INT_VAL(load_ptr(clone->bcLength)));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("maximumNumberOfStackElements differs!!",
            INT_VAL(load_ptr(orig->maximumNumberOfStackElements)),
            INT_VAL(load_ptr(clone->maximumNumberOfStackElements)));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfArguments differs!!",
            INT_VAL(load_ptr(orig->numberOfArguments)),
            INT_VAL(load_ptr(clone->numberOfArguments)));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfConstants differs!!",
            INT_VAL(load_ptr(orig->numberOfConstants)),
            INT_VAL(load_ptr(clone->numberOfConstants)));

    CPPUNIT_ASSERT_EQUAL_MESSAGE("GetHolder() differs!!", orig->GetHolder(), clone->GetHolder());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("GetSignature() differs!!", orig->GetSignature(), clone->GetSignature());
}

void CloneObjectsTest::testCloneClass() {
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    VMClass* orig = GetUniverse()->NewClass(load_ptr(integerClass), page);
    orig->SetName(GetUniverse()->NewSymbol("MyClass", page));
    orig->SetSuperClass(load_ptr(doubleClass));
    orig->SetInstanceFields(GetUniverse()->NewArray(2, page));
    orig->SetInstanceInvokables(GetUniverse()->NewArray(4, page));
    VMClass* clone = orig->Clone(page);

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->clazz, clone->clazz);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->objectSize, clone->objectSize);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!", orig->numberOfFields, clone->numberOfFields);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("superClass differs!!", orig->superClass, clone->superClass);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("name differs!!", orig->name, clone->name);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("instanceFields differs!!", orig->instanceFields, clone->instanceFields);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("instanceInvokables differs!!", orig->instanceInvokables, clone->instanceInvokables);
}
