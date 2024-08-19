#pragma once

#include <cppunit/extensions/HelperMacros.h>

#include "../vmobjects/VMArray.h"

using namespace std;

class WriteBarrierTest : public CPPUNIT_NS::TestCase {
    CPPUNIT_TEST_SUITE(WriteBarrierTest);  // NOLINT(misc-const-correctness)
    CPPUNIT_TEST(testWriteArray);
    CPPUNIT_TEST(testWriteClass);
    CPPUNIT_TEST(testWriteBlock);
    CPPUNIT_TEST(testWriteFrame);
    CPPUNIT_TEST(testWriteEvaluationPrimitive);
    CPPUNIT_TEST(testWriteMethod);
    CPPUNIT_TEST_SUITE_END();

public:
    inline void setUp() {}
    inline void tearDown() {}

private:
    static void testWriteArray();
    static void testWriteClass();
    static void testWriteBlock();
    static void testWriteFrame();
    static void testWriteMethod();
    static void testWriteEvaluationPrimitive();
};
