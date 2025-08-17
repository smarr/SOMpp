#pragma once

#include <cppunit/extensions/HelperMacros.h>

#include "../vmobjects/VMArray.h"

using namespace std;

class InfIntTest : public CPPUNIT_NS::TestCase {
    CPPUNIT_TEST_SUITE(InfIntTest);  // NOLINT(misc-const-correctness)
    CPPUNIT_TEST(testBasicNumbers);
    CPPUNIT_TEST(testIsZero);
    CPPUNIT_TEST(testIsWithinSmallIntRange);
    CPPUNIT_TEST_SUITE_END();

public:
    inline void setUp() override {}
    inline void tearDown() override {}

private:
    static void testBasicNumbers();
    static void testIsZero();
    static void testIsWithinSmallIntRange();
};
