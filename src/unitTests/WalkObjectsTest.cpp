/*
 * WalkObjectsTest.cpp
 *
 *  Created on: 12.01.2011
 *      Author: christian
 */
#include <vector>
#include <algorithm>

#define private public
#include "WalkObjectsTest.h"
#include "vmobjects/VMSymbol.h"
#include "vmobjects/VMClass.h"
#include "vmobjects/VMDouble.h"
#include "vmobjects/VMInteger.h"
#include "vmobjects/VMArray.h"
#include "vmobjects/VMMethod.h"
#include "vmobjects/VMBlock.h"
#include "vmobjects/VMPrimitive.h"
#include "vmobjects/VMFrame.h"
#include "vmobjects/VMEvaluationPrimitive.h"

static const size_t NoOfFields_Object = 1;
static const size_t NoOfFields_String = 0;
static const size_t NoOfFields_Symbol = 0;
static const size_t NoOfFields_Double = 0;
static const size_t NoOfFields_Integer = 0;
static const size_t NoOfFields_BigInteger = 0;
static const size_t NoOfFields_Array = NoOfFields_Object;
static const size_t NoOfFields_Invokable = 2 + NoOfFields_Object;
static const size_t NoOfFields_Method = 5 + NoOfFields_Invokable;
static const size_t NoOfFields_Class = 4 + NoOfFields_Object;
static const size_t NoOfFields_Frame = 3 + NoOfFields_Array;
static const size_t NoOfFields_Block = 2 + NoOfFields_Object;
static const size_t NoOfFields_Primitive = NoOfFields_Invokable;
static const size_t NoOfFields_EvaluationPrimitive = 1 + NoOfFields_Primitive;

static vector<gc_oop_t> walkedObjects;
/*
 * This method simply pushes all objects into the vector walkedObjects
 */
gc_oop_t collectMembers(gc_oop_t obj) {
    walkedObjects.push_back(obj);
    return obj;
}
/*
 * Helper function that searches the result vector for a field
 */
bool WalkerHasFound(gc_oop_t obj) {
    return find(walkedObjects.begin(), walkedObjects.end(), obj)
    != walkedObjects.end();
}

void WalkObjectsTest::testWalkInteger() {
    walkedObjects.clear();
    VMInteger* int1 = GetUniverse()->NewInteger(42);
    int1->WalkObjects(collectMembers);

    //Integers have no additional members
    CPPUNIT_ASSERT_EQUAL(NoOfFields_Integer, walkedObjects.size());
}

void WalkObjectsTest::testWalkDouble() {
    walkedObjects.clear();
    VMDouble* d1 = GetUniverse()->NewDouble(432.1);
    d1->WalkObjects(collectMembers);

    //Doubles have no additional members
    CPPUNIT_ASSERT_EQUAL(NoOfFields_Double, walkedObjects.size());
}

void WalkObjectsTest::testWalkEvaluationPrimitive() {
    walkedObjects.clear();

    VMEvaluationPrimitive* evPrim = new (GetHeap<HEAP_CLS>()) VMEvaluationPrimitive(1);
    evPrim->WalkObjects(collectMembers);

    CPPUNIT_ASSERT(WalkerHasFound(evPrim->numberOfArguments));
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(evPrim->GetClass())));
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(evPrim->GetSignature())));
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(evPrim->GetHolder())));
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(evPrim)));
    CPPUNIT_ASSERT_EQUAL(NoOfFields_EvaluationPrimitive + 1, walkedObjects.size());
}

void WalkObjectsTest::testWalkObject() {
    walkedObjects.clear();

    VMObject* obj = new (GetHeap<HEAP_CLS>()) VMObject();
    obj->WalkObjects(collectMembers);

    //Objects should only have one member -> Class
    CPPUNIT_ASSERT_EQUAL(NoOfFields_Object, walkedObjects.size());
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(obj->GetClass())));
}

void WalkObjectsTest::testWalkString() {
    walkedObjects.clear();
    VMString* str1 = GetUniverse()->NewString("str1");
    str1->WalkObjects(collectMembers);

    CPPUNIT_ASSERT_EQUAL(NoOfFields_String, walkedObjects.size());
}

void WalkObjectsTest::testWalkSymbol() {
    walkedObjects.clear();
    VMSymbol* sym = GetUniverse()->NewSymbol("symbol");
    sym->WalkObjects(collectMembers);

    CPPUNIT_ASSERT_EQUAL(NoOfFields_Symbol, walkedObjects.size());
}

void WalkObjectsTest::testWalkClass() {
    walkedObjects.clear();
    VMClass* meta = GetUniverse()->NewMetaclassClass();
    meta->superClass = stringClass;
    meta->WalkObjects(collectMembers);

    //Now check if we found all class fields
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(meta->GetClass())));
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(meta->GetSuperClass())));
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(meta->GetName())));
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(meta->GetInstanceFields())));
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(meta->GetInstanceInvokables())));
    CPPUNIT_ASSERT_EQUAL(NoOfFields_Class, walkedObjects.size());
}

void WalkObjectsTest::testWalkPrimitive() {
    walkedObjects.clear();
    VMSymbol* primitiveSymbol = GetUniverse()->NewSymbol("myPrimitive");
    VMPrimitive* prim = VMPrimitive::GetEmptyPrimitive(primitiveSymbol, false);

    prim->WalkObjects(collectMembers);
    CPPUNIT_ASSERT_EQUAL(NoOfFields_Primitive, walkedObjects.size());
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(prim->GetClass())));
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(prim->GetSignature())));
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(prim->GetHolder())));
}

void WalkObjectsTest::testWalkFrame() {
    walkedObjects.clear();
    VMSymbol* methodSymbol = GetUniverse()->NewSymbol("frameMethod");
    VMMethod* method = GetUniverse()->NewMethod(methodSymbol, 0, 0);
    VMFrame* frame = GetUniverse()->NewFrame(nullptr, method);
    frame->SetPreviousFrame(frame->Clone());
    frame->SetContext(frame->Clone());
    VMInteger* dummyArg = GetUniverse()->NewInteger(1111);
    frame->SetArgument(0, 0, dummyArg);
    frame->WalkObjects(collectMembers);

    // CPPUNIT_ASSERT(WalkerHasFound(frame->GetClass()));  // VMFrame does no longer have a SOM representation
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(frame->GetPreviousFrame())));
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(frame->GetContext())));
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(frame->GetMethod())));
    //CPPUNIT_ASSERT(WalkerHasFound(frame->bytecodeIndex));
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(dummyArg)));
    CPPUNIT_ASSERT_EQUAL(
            (long) (NoOfFields_Frame + method->GetNumberOfArguments()),
            (long) walkedObjects.size() + 1);  // + 1 for the class field that's still in there
}

void WalkObjectsTest::testWalkMethod() {
    walkedObjects.clear();
    VMSymbol* methodSymbol = GetUniverse()->NewSymbol("myMethod");
    VMMethod* method = GetUniverse()->NewMethod(methodSymbol, 0, 0);
    method->WalkObjects(collectMembers);

    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(method->GetClass())));
    //the following fields had no getters -> had to become friend
    CPPUNIT_ASSERT(WalkerHasFound(method->numberOfLocals));
    CPPUNIT_ASSERT(WalkerHasFound(method->bcLength));
    CPPUNIT_ASSERT(WalkerHasFound(method->maximumNumberOfStackElements));
    CPPUNIT_ASSERT(WalkerHasFound(method->numberOfArguments));
    CPPUNIT_ASSERT(WalkerHasFound(method->numberOfConstants));
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(method->GetHolder())));
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(method->GetSignature())));
    CPPUNIT_ASSERT_EQUAL(NoOfFields_Method, walkedObjects.size());
}

void WalkObjectsTest::testWalkBlock() {
    walkedObjects.clear();
    VMSymbol* methodSymbol = GetUniverse()->NewSymbol("someMethod");
    VMMethod* method = GetUniverse()->NewMethod(methodSymbol, 0, 0);
    VMBlock* block = GetUniverse()->NewBlock(method,
            GetUniverse()->GetInterpreter()->GetFrame(),
            method->GetNumberOfArguments());
    block->WalkObjects(collectMembers);
    CPPUNIT_ASSERT_EQUAL(NoOfFields_Block, walkedObjects.size());
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(block->GetClass())));
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(block->GetContext())));
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(method)));
}

void WalkObjectsTest::testWalkArray() {
    walkedObjects.clear();
    VMString* str1 = GetUniverse()->NewString("str1");
    VMInteger* int1 = GetUniverse()->NewInteger(42);
    VMArray* a = GetUniverse()->NewArray(2);
    a->SetIndexableField(0, str1);
    a->SetIndexableField(1, int1);
    a->WalkObjects(collectMembers);

    CPPUNIT_ASSERT_EQUAL(NoOfFields_Array + 2, walkedObjects.size());
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(a->GetClass())));
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(str1)));
    CPPUNIT_ASSERT(WalkerHasFound(_store_ptr(int1)));
}
