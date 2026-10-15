#pragma once

#include <cppunit/extensions/HelperMacros.h>

class ClassFieldsTest : public CPPUNIT_NS::TestCase {
    CPPUNIT_TEST_SUITE(ClassFieldsTest);  // NOLINT(misc-const-correctness)
    CPPUNIT_TEST(testClassWithoutClassFields);
    CPPUNIT_TEST(testClassFieldsFollowFieldsOfVMClass);
    CPPUNIT_TEST_SUITE_END();

private:
    static void testClassWithoutClassFields();
    static void testClassFieldsFollowFieldsOfVMClass();
};
