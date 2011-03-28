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
#include "vmobjects/VMBigInteger.h"
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
static const size_t NoOfFields_Frame = 6 + NoOfFields_Array;
static const size_t NoOfFields_Block = 2 + NoOfFields_Object;
static const size_t NoOfFields_Primitive = NoOfFields_Invokable;
static const size_t NoOfFields_EvaluationPrimitive = 1 + NoOfFields_Primitive;

static vector<pVMObject> walkedObjects;
/*
 * This method simply pushes all objects into the vector walkedObjects
 */
pVMObject collectMembers(pVMObject obj) {
	walkedObjects.push_back(obj);
	return obj;
}
/*
 * Helper function that searches the result vector for a field
 */
bool WalkerHasFound(pVMObject obj) {
	return find(walkedObjects.begin(), walkedObjects.end(), obj)
			!= walkedObjects.end();
}

void WalkObjectsTest::testWalkInteger() {
	walkedObjects.clear();
	pVMInteger int1 = _UNIVERSE->NewInteger(42);
	int1->WalkObjects(collectMembers);

	//Integers have no additional members
	CPPUNIT_ASSERT_EQUAL(NoOfFields_Integer, walkedObjects.size());
}

void WalkObjectsTest::testWalkBigInteger() {
	walkedObjects.clear();
	pVMBigInteger int1 = _UNIVERSE->NewBigInteger(4711);
	int1->WalkObjects(collectMembers);

	//Integers have no additional members
	CPPUNIT_ASSERT_EQUAL(NoOfFields_BigInteger, walkedObjects.size());
}

void WalkObjectsTest::testWalkDouble() {
	walkedObjects.clear();
	pVMDouble d1 = _UNIVERSE->NewDouble(432.1);
	d1->WalkObjects(collectMembers);

	//Doubles have no additional members
	CPPUNIT_ASSERT_EQUAL(NoOfFields_Double, walkedObjects.size());
}

void WalkObjectsTest::testWalkEvaluationPrimitive() {
	walkedObjects.clear();

	pVMEvaluationPrimitive evPrim = new (_UNIVERSE->GetHeap()) VMEvaluationPrimitive(1);
	evPrim->WalkObjects(collectMembers);

	CPPUNIT_ASSERT(WalkerHasFound(evPrim->numberOfArguments));
	CPPUNIT_ASSERT(WalkerHasFound(evPrim->GetClass()));
	CPPUNIT_ASSERT(WalkerHasFound(evPrim->GetSignature()));
	CPPUNIT_ASSERT(WalkerHasFound(evPrim->GetHolder()));
	CPPUNIT_ASSERT_EQUAL(NoOfFields_EvaluationPrimitive, walkedObjects.size());
}

void WalkObjectsTest::testWalkObject() {
	walkedObjects.clear();

	pVMObject obj = new (_UNIVERSE->GetHeap()) VMObject();
	obj->WalkObjects(collectMembers);

	//Objects should only have one member -> Class
	CPPUNIT_ASSERT_EQUAL(NoOfFields_Object, walkedObjects.size());
	CPPUNIT_ASSERT(WalkerHasFound(obj->GetClass()));
}

void WalkObjectsTest::testWalkString() {
	walkedObjects.clear();
	pVMString str1 = _UNIVERSE->NewString("str1");
	str1->WalkObjects(collectMembers);

	CPPUNIT_ASSERT_EQUAL(NoOfFields_String, walkedObjects.size());
}

void WalkObjectsTest::testWalkSymbol() {
	walkedObjects.clear();
	pVMSymbol sym = _UNIVERSE->NewSymbol("symbol");
	sym->WalkObjects(collectMembers);

	CPPUNIT_ASSERT_EQUAL(NoOfFields_Symbol, walkedObjects.size());
}

void WalkObjectsTest::testWalkClass() {
	walkedObjects.clear();
	pVMClass meta = _UNIVERSE->NewMetaclassClass();
	meta->WalkObjects(collectMembers);

	//Now check if we found all class fields
	CPPUNIT_ASSERT_EQUAL(NoOfFields_Class, walkedObjects.size());
	CPPUNIT_ASSERT(WalkerHasFound(meta->GetClass()));
	CPPUNIT_ASSERT(WalkerHasFound(meta->GetSuperClass()));
	CPPUNIT_ASSERT(WalkerHasFound(meta->GetName()));
	CPPUNIT_ASSERT(WalkerHasFound(meta->GetInstanceFields()));
	CPPUNIT_ASSERT(WalkerHasFound(meta->GetInstanceInvokables()));
}

void WalkObjectsTest::testWalkPrimitive() {
	walkedObjects.clear();
	pVMSymbol primitiveSymbol = _UNIVERSE->NewSymbol("myPrimitive");
	pVMPrimitive prim = VMPrimitive::GetEmptyPrimitive(primitiveSymbol);

	prim->WalkObjects(collectMembers);
	CPPUNIT_ASSERT_EQUAL(NoOfFields_Primitive, walkedObjects.size());
	CPPUNIT_ASSERT(WalkerHasFound(prim->GetClass()));
	CPPUNIT_ASSERT(WalkerHasFound(prim->GetSignature()));
	CPPUNIT_ASSERT(WalkerHasFound(prim->GetHolder()));
}

void WalkObjectsTest::testWalkFrame() {
	walkedObjects.clear();
	pVMSymbol methodSymbol = _UNIVERSE->NewSymbol("frameMethod");
	pVMMethod method = _UNIVERSE->NewMethod(methodSymbol, 0, 0);
	pVMFrame frame = _UNIVERSE->NewFrame(NULL, method);
	pVMInteger dummyArg = _UNIVERSE->NewInteger(1111);
	frame->SetArgument(0, 0, dummyArg);
	frame->WalkObjects(collectMembers);

	CPPUNIT_ASSERT(WalkerHasFound(frame->GetClass()));
	CPPUNIT_ASSERT(WalkerHasFound(frame->GetPreviousFrame()));
	CPPUNIT_ASSERT(WalkerHasFound(frame->GetContext()));
	CPPUNIT_ASSERT(WalkerHasFound(frame->GetMethod()));
	CPPUNIT_ASSERT(WalkerHasFound(frame->GetStackPointer()));
	CPPUNIT_ASSERT(WalkerHasFound(frame->bytecodeIndex));
	CPPUNIT_ASSERT(WalkerHasFound(frame->localOffset));
	CPPUNIT_ASSERT(WalkerHasFound(dummyArg));
	CPPUNIT_ASSERT_EQUAL(NoOfFields_Frame + method->GetNumberOfArguments(), walkedObjects.size());
}

void WalkObjectsTest::testWalkMethod() {
	walkedObjects.clear();
	pVMSymbol methodSymbol = _UNIVERSE->NewSymbol("myMethod");
	pVMMethod method = _UNIVERSE->NewMethod(methodSymbol, 0, 0);
	method->WalkObjects(collectMembers);

	CPPUNIT_ASSERT(WalkerHasFound(method->GetClass()));
	//the following fields had no getters -> had to become friend
	CPPUNIT_ASSERT(WalkerHasFound(method->numberOfLocals));
	CPPUNIT_ASSERT(WalkerHasFound(method->bcLength));
	CPPUNIT_ASSERT(WalkerHasFound(method->maximumNumberOfStackElements));
	CPPUNIT_ASSERT(WalkerHasFound(method->numberOfArguments));
	CPPUNIT_ASSERT(WalkerHasFound(method->numberOfConstants));
	CPPUNIT_ASSERT(WalkerHasFound(method->GetHolder()));
	CPPUNIT_ASSERT(WalkerHasFound(method->GetSignature()));
	CPPUNIT_ASSERT_EQUAL(NoOfFields_Method, walkedObjects.size());
}

void WalkObjectsTest::testWalkBlock() {
	walkedObjects.clear();
	pVMSymbol methodSymbol = _UNIVERSE->NewSymbol("someMethod");
	pVMMethod method = _UNIVERSE->NewMethod(methodSymbol, 0, 0);
	pVMBlock block = _UNIVERSE->NewBlock(method,
			_UNIVERSE->GetInterpreter()->GetFrame(),
			method->GetNumberOfArguments());
	block->WalkObjects(collectMembers);
	CPPUNIT_ASSERT_EQUAL(NoOfFields_Block, walkedObjects.size());
	CPPUNIT_ASSERT(WalkerHasFound(block->GetClass()));
	CPPUNIT_ASSERT(WalkerHasFound(block->GetContext()));
	CPPUNIT_ASSERT(WalkerHasFound(method));
}

void WalkObjectsTest::testWalkArray() {
	walkedObjects.clear();
	pVMString str1 = _UNIVERSE->NewString("str1");
	pVMInteger int1 = _UNIVERSE->NewInteger(42);
	pVMArray a = _UNIVERSE->NewArray(2);
	(*a)[0] = str1;
	(*a)[1] = int1;
	a->WalkObjects(collectMembers);

	CPPUNIT_ASSERT_EQUAL(NoOfFields_Array + 2, walkedObjects.size());
	CPPUNIT_ASSERT(WalkerHasFound(a->GetClass()));
	CPPUNIT_ASSERT(WalkerHasFound(str1));
	CPPUNIT_ASSERT(WalkerHasFound(int1));
}
