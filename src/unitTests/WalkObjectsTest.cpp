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
void collectMembers(gc_oop_t* obj_p, Page*) {
    walkedObjects.push_back(*obj_p);
}
/*
 * Helper function that searches the result vector for a field
 */
bool WalkerHasFound(gc_oop_t obj) {
    return find(walkedObjects.begin(), walkedObjects.end(), obj)
    != walkedObjects.end();
}

void WalkObjectsTest::testWalkInteger() {
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    walkedObjects.clear();
    VMInteger* int1 = GetUniverse()->NewInteger(42, page);
    int1->WalkObjects(collectMembers, page);

    //Integers have no additional members
    CPPUNIT_ASSERT_EQUAL(NoOfFields_Integer, walkedObjects.size());
}

void WalkObjectsTest::testWalkDouble() {
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    walkedObjects.clear();
    VMDouble* d1 = GetUniverse()->NewDouble(432.1, page);
    d1->WalkObjects(collectMembers, page);

    //Doubles have no additional members
    CPPUNIT_ASSERT_EQUAL(NoOfFields_Double, walkedObjects.size());
}

void WalkObjectsTest::testWalkEvaluationPrimitive() {
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    walkedObjects.clear();

    VMEvaluationPrimitive* evPrim = new (page) VMEvaluationPrimitive(1, page);
    evPrim->WalkObjects(collectMembers, page);

    CPPUNIT_ASSERT(WalkerHasFound(evPrim->numberOfArguments));
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(evPrim->GetClass())));
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(evPrim->GetSignature())));
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(evPrim->GetHolder())));
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(evPrim)));
    CPPUNIT_ASSERT_EQUAL(NoOfFields_EvaluationPrimitive + 1, walkedObjects.size());
}

void WalkObjectsTest::testWalkObject() {
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    walkedObjects.clear();

    VMObject* obj = new (page) VMObject();
    obj->WalkObjects(collectMembers, page);

    //Objects should only have one member -> Class
    CPPUNIT_ASSERT_EQUAL(NoOfFields_Object, walkedObjects.size());
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(obj->GetClass())));
}

void WalkObjectsTest::testWalkString() {
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    walkedObjects.clear();
    VMString* str1 = GetUniverse()->NewString("str1", page);
    str1->WalkObjects(collectMembers, page);

    CPPUNIT_ASSERT_EQUAL(NoOfFields_String, walkedObjects.size());
}

void WalkObjectsTest::testWalkSymbol() {
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    walkedObjects.clear();
    VMSymbol* sym = GetUniverse()->NewSymbol("symbol", page);
    sym->WalkObjects(collectMembers, page);

    CPPUNIT_ASSERT_EQUAL(NoOfFields_Symbol, walkedObjects.size());
}

void WalkObjectsTest::testWalkClass() {
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    walkedObjects.clear();
    VMClass* meta = GetUniverse()->NewMetaclassClass(page);
    meta->superClass = stringClass;
    meta->WalkObjects(collectMembers, page);

    //Now check if we found all class fields
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(meta->GetClass())));
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(meta->GetSuperClass())));
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(meta->GetName())));
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(meta->GetInstanceFields())));
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(meta->GetInstanceInvokables())));
    CPPUNIT_ASSERT_EQUAL(NoOfFields_Class, walkedObjects.size());
}

void WalkObjectsTest::testWalkPrimitive() {
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    walkedObjects.clear();
    VMSymbol* primitiveSymbol = GetUniverse()->NewSymbol("myPrimitive", page);
    VMPrimitive* prim = VMPrimitive::GetEmptyPrimitive(primitiveSymbol, false, page);

    prim->WalkObjects(collectMembers, page);
    CPPUNIT_ASSERT_EQUAL(NoOfFields_Primitive, walkedObjects.size());
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(prim->GetClass())));
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(prim->GetSignature())));
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(prim->GetHolder())));
}

void WalkObjectsTest::testWalkFrame() {
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    walkedObjects.clear();
    VMSymbol* methodSymbol = GetUniverse()->NewSymbol("frameMethod", page);
    VMMethod* method = GetUniverse()->NewMethod(methodSymbol, 0, 0, page);
    VMFrame* frame = GetUniverse()->NewFrame(nullptr, method, page);
    frame->SetPreviousFrame(frame->Clone(page));
    frame->SetContext(frame->Clone(page));
    VMInteger* dummyArg = GetUniverse()->NewInteger(1111, page);
    frame->SetArgument(0, 0, dummyArg);
    frame->WalkObjects(collectMembers, page);

    // CPPUNIT_ASSERT(WalkerHasFound(frame->GetClass()));  // VMFrame does no longer have a SOM representation
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(frame->GetPreviousFrame())));
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(frame->GetContext())));
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(frame->GetMethod())));
    //CPPUNIT_ASSERT(WalkerHasFound(frame->bytecodeIndex));
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(dummyArg)));
    CPPUNIT_ASSERT_EQUAL(
            (long) (NoOfFields_Frame + method->GetNumberOfArguments()),
            (long) walkedObjects.size() + 1);  // + 1 for the class field that's still in there
}

void WalkObjectsTest::testWalkMethod() {
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    walkedObjects.clear();
    VMSymbol* methodSymbol = GetUniverse()->NewSymbol("myMethod", page);
    VMMethod* method = GetUniverse()->NewMethod(methodSymbol, 0, 0, page);
    method->WalkObjects(collectMembers, page);

    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(method->GetClass())));
    //the following fields had no getters -> had to become friend
    CPPUNIT_ASSERT(WalkerHasFound(method->numberOfLocals));
    CPPUNIT_ASSERT(WalkerHasFound(method->bcLength));
    CPPUNIT_ASSERT(WalkerHasFound(method->maximumNumberOfStackElements));
    CPPUNIT_ASSERT(WalkerHasFound(method->numberOfArguments));
    CPPUNIT_ASSERT(WalkerHasFound(method->numberOfConstants));
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(method->GetHolder())));
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(method->GetSignature())));
    CPPUNIT_ASSERT_EQUAL(NoOfFields_Method, walkedObjects.size());
}

void WalkObjectsTest::testWalkBlock() {
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    walkedObjects.clear();
    VMSymbol* methodSymbol = GetUniverse()->NewSymbol("someMethod", page);
    VMMethod* method = GetUniverse()->NewMethod(methodSymbol, 0, 0, page);
    VMBlock* block = GetUniverse()->NewBlock(method,
            GetUniverse()->NewFrame(nullptr, method, page),
            method->GetNumberOfArguments(), page);
    block->WalkObjects(collectMembers, page);
    CPPUNIT_ASSERT_EQUAL(NoOfFields_Block, walkedObjects.size());
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(block->GetClass())));
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(block->GetContext())));
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(method)));
}

void WalkObjectsTest::testWalkArray() {
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    walkedObjects.clear();
    VMString* str1 = GetUniverse()->NewString("str1", page);
    VMInteger* int1 = GetUniverse()->NewInteger(42, page);
    VMArray* a = GetUniverse()->NewArray(2, page);
    a->SetIndexableField(0, str1);
    a->SetIndexableField(1, int1);
    a->WalkObjects(collectMembers, page);

    CPPUNIT_ASSERT_EQUAL(NoOfFields_Array + 2, walkedObjects.size());
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(a->GetClass())));
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(str1)));
    CPPUNIT_ASSERT(WalkerHasFound(to_gc_ptr(int1)));
}
