/*
 * WalkObjectsTest.cpp
 *
 *  Created on: 12.01.2011
 *      Author: christian
 */
#include <vector>
#include <algorithm>

#include "WalkObjectsTest.h"
#include "vmobjects/VMArray.h"

static const int32_t NoOfFields_Object = 1;
static const int32_t NoOfFields_String = 0;
static const int32_t NoOfFields_Integer = 0;


static vector<pVMObject> walkedObjects;
/*
 * This method simply pushes all objects into the vector walkedObjects
 */
pVMObject collectMembers(pVMObject obj) {
	walkedObjects.push_back(obj);
}
/*
 * Helper function that searches the result vector for a field
 */
bool WalkerHasFound(pVMObject obj) {
	return find(walkedObjects.begin(), walkedObjects.end(), obj) != walkedObjects.end();
}

void WalkObjectsTest::testWalkInteger() {
	walkedObjects.clear();
	pVMInteger int1 = _UNIVERSE->NewInteger(42);
	int1->WalkObjects(collectMembers);
	//Integers should only have one member -> Class
	CPPUNIT_ASSERT(walkedObjects.size() == NoOfFields_Object + NoOfFields_String);
	CPPUNIT_ASSERT(walkedObjects[0] == int1->GetClass());
}

void WalkObjectsTest::testWalkBigInteger() {
	walkedObjects.clear();
	pVMBigInteger int1 = _UNIVERSE->NewBigInteger(4711);
	int1->WalkObjects(collectMembers);
	//BigIntegers should only have one member -> Class
	CPPUNIT_ASSERT(walkedObjects.size() == NoOfFields_Object + NoOfFields_String);
	CPPUNIT_ASSERT(walkedObjects[0] == int1->GetClass());
}

void WalkObjectsTest::testWalkString() {
	walkedObjects.clear();
	pVMString str1 = _UNIVERSE->NewString("str1");
	str1->WalkObjects(collectMembers);
	//Strings should only have one member -> Class
	CPPUNIT_ASSERT(walkedObjects.size() == 1);
	CPPUNIT_ASSERT(WalkerHasFound(str1->GetClass()));
}

void WalkObjectsTest::testWalkArray() {
	walkedObjects.clear();
	pVMString str1 = _UNIVERSE->NewString("str1");
	pVMInteger int1 = _UNIVERSE->NewInteger(42);
	pVMArray a = _UNIVERSE->NewArray(2);
	a->SetField(0, str1);
	a->SetField(1, int1);
	a->WalkObjects(collectMembers);
	CPPUNIT_ASSERT(walkedObjects.size() == NoOfFields_Object + 2);
	CPPUNIT_ASSERT(WalkerHasFound(a->GetClass()));
	CPPUNIT_ASSERT(WalkerHasFound(str1));
	CPPUNIT_ASSERT(WalkerHasFound(int1));
}
