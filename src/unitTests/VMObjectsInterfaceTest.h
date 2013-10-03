/*
 * VMObjectsInterfaceTest.h
 *
 *  Created on: 02.03.2011
 *      Author: christian
 */

#ifndef VMOBJECTSINTERFACETEST_H_
#define VMOBJECTSINTERFACETEST_H_

#include <cppunit/extensions/HelperMacros.h>

class VMObject;
class VMInteger;
class VMDouble;
class VMString;
class VMSymbol;
class VMArray;
class VMBlock;
class VMPrimitive;
class VMBigInteger;
class VMClass;
class VMFrame;
class VMMethod;
class VMEvaluationPrimitive;

class VMObjectsInterfaceTest: public CPPUNIT_NS::TestCase {
    CPPUNIT_TEST_SUITE (VMObjectsInterfaceTest);
    CPPUNIT_TEST (testGetSetObjectSize);
    CPPUNIT_TEST (testGetSetClass);
    CPPUNIT_TEST (testGetSetGCField);
    CPPUNIT_TEST (testGetNumberOfFields);
    CPPUNIT_TEST (testGetHash);
    CPPUNIT_TEST (testGetSetField);
    CPPUNIT_TEST (testGetFieldName);
    CPPUNIT_TEST (testGetClassField);CPPUNIT_TEST_SUITE_END();

public:
    void setUp(void);
    void tearDown(void);

private:
    VMObject* pObject;
    VMInteger* pInteger;
    VMDouble* pDouble;
    VMString* pString;
    VMSymbol* pSymbol;
    VMArray* pArray;
    VMArray* pArray3;
    VMBlock* pBlock;
    VMPrimitive* pPrimitive;
    VMBigInteger* pBigInteger;
    VMClass* pClass;
    VMFrame* pFrame;
    VMMethod* pMethod;
    VMEvaluationPrimitive* pEvaluationPrimitive;
    void testGetSetGCField();
    void testGetSetField();
    void testGetSetObjectSize();
    void testGetNumberOfFields();
    void testGetSetClass();
    void testGetFieldName();
    void testGetHash();
    void testGetClassField();
};

#endif /* VMOBJECTSINTERFACETEST_H_ */
