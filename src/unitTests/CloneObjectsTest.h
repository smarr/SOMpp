#pragma once
/*
 * CloneObjectsTest.h
 *
 *  Created on: 21.01.2011
 *      Author: christian
 */

#include <cppunit/extensions/HelperMacros.h>

class CloneObjectsTest : public CPPUNIT_NS::TestCase {
    CPPUNIT_TEST_SUITE(CloneObjectsTest);  // NOLINT(misc-const-correctness)
    CPPUNIT_TEST(testCloneObject);
    CPPUNIT_TEST(testCloneInteger);
    CPPUNIT_TEST(testCloneDouble);
    CPPUNIT_TEST(testCloneString);
    CPPUNIT_TEST(testCloneSymbol);
    CPPUNIT_TEST(testCloneArray);
    CPPUNIT_TEST(testCloneMethod);
    CPPUNIT_TEST(testCloneBlock);
    CPPUNIT_TEST(testClonePrimitive);
    CPPUNIT_TEST(testCloneClass);
    CPPUNIT_TEST(testCloneFrame);
    CPPUNIT_TEST(testCloneEvaluationPrimitive);
    CPPUNIT_TEST_SUITE_END();

public:
    inline void setUp() override {}
    inline void tearDown() override {}

private:
    static void testCloneObject();
    static void testCloneInteger();
    static void testCloneDouble();
    static void testCloneString();
    static void testCloneSymbol();
    static void testCloneArray();
    static void testCloneBlock();
    static void testClonePrimitive();
    static void testCloneClass();
    static void testCloneFrame();
    static void testCloneMethod();
    static void testCloneEvaluationPrimitive();
};
