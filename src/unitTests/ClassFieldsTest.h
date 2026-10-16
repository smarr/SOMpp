#pragma once

#include <cppunit/extensions/HelperMacros.h>

#include "../vmobjects/ObjectFormats.h"

class ClassFieldsTest : public CPPUNIT_NS::TestCase {
    CPPUNIT_TEST_SUITE(ClassFieldsTest);  // NOLINT(misc-const-correctness)
    CPPUNIT_TEST(testClassWithoutClassFields);
    CPPUNIT_TEST(testClassFieldsFollowFieldsOfVMClass);
    CPPUNIT_TEST(testSuperclassFieldsAreCountedOnce);
    CPPUNIT_TEST(testFieldNamesOfSubclass);
    CPPUNIT_TEST(testClassSideFieldsAreCountedOnce);
    CPPUNIT_TEST(testClassSideFieldNamesOfSubclass);
    CPPUNIT_TEST_SUITE_END();

private:
    static void testClassWithoutClassFields();
    static void testClassFieldsFollowFieldsOfVMClass();
    static void testSuperclassFieldsAreCountedOnce();
    static void testFieldNamesOfSubclass();
    static void testClassSideFieldsAreCountedOnce();
    static void testClassSideFieldNamesOfSubclass();

    static VMClass* compileSubclass();
};
