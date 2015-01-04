#pragma once
/*
 * CloneObjectsTest.h
 *
 *  Created on: 21.01.2011
 *      Author: christian
 */

#include <cppunit/extensions/HelperMacros.h>

class CloneObjectsTest: public CPPUNIT_NS::TestCase {
    CPPUNIT_TEST_SUITE (CloneObjectsTest);
    CPPUNIT_TEST (testCloneObject);
    CPPUNIT_TEST (testCloneInteger);
    CPPUNIT_TEST (testCloneDouble);
    CPPUNIT_TEST (testCloneString);
    CPPUNIT_TEST (testCloneSymbol);
    CPPUNIT_TEST (testCloneArray);
    CPPUNIT_TEST (testCloneMethod);
    CPPUNIT_TEST (testCloneBlock);
    CPPUNIT_TEST (testClonePrimitive);
    CPPUNIT_TEST (testCloneClass);
    CPPUNIT_TEST (testCloneFrame);
    CPPUNIT_TEST (testCloneEvaluationPrimitive);CPPUNIT_TEST_SUITE_END();

public:
    inline void setUp(void) {
    }
    inline void tearDown(void) {
    }
private:
    void testCloneObject();
    void testCloneInteger();
    void testCloneDouble();
    void testCloneString();
    void testCloneSymbol();
    void testCloneArray();
    void testCloneBlock();
    void testClonePrimitive();
    void testCloneClass();
    void testCloneFrame();
    void testCloneMethod();
    void testCloneEvaluationPrimitive();
};
