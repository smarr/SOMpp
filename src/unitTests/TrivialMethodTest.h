#pragma once

#include <cppunit/extensions/HelperMacros.h>

#include "TestWithParsing.h"

class TrivialMethodTest : public TestWithParsing {
    CPPUNIT_TEST_SUITE(TrivialMethodTest);  // NOLINT(misc-const-correctness)
    CPPUNIT_TEST(testLiteralReturn);
    CPPUNIT_TEST(testLiteralNoReturn);
    CPPUNIT_TEST(testBlockLiteralReturn);
    CPPUNIT_TEST(testNonTrivialLiteralReturn);
    CPPUNIT_TEST(testGlobalReturn);
    CPPUNIT_TEST(testNonTrivialGlobalReturn);
    CPPUNIT_TEST(testUnknownGlobalInBlock);
    CPPUNIT_TEST(testFieldGetter0);
    CPPUNIT_TEST(testFieldGetterN);
    CPPUNIT_TEST(testNonTrivialFieldGetter0);
    CPPUNIT_TEST(testNonTrivialFieldGetterN);
    CPPUNIT_TEST(testFieldSetter0);
    CPPUNIT_TEST(testFieldSetterN);
    CPPUNIT_TEST(testNonTrivialFieldSetter0);
    CPPUNIT_TEST(testNonTrivialFieldSetterN);
    CPPUNIT_TEST(testBlockReturn);
    CPPUNIT_TEST_SUITE_END();

private:
    void testLiteralReturn();
    void literalReturn(const std::string& source);

    void testBlockLiteralReturn();
    void blockLiteralReturn(const std::string& source);

    void testLiteralNoReturn();
    void literalNoReturn(const std::string& source);

    void testNonTrivialLiteralReturn();
    void nonTrivialLiteralReturn(const std::string& source);

    void testGlobalReturn();
    void globalReturn(const std::string& source);

    void testNonTrivialGlobalReturn();
    void testUnknownGlobalInBlock();

    void testFieldGetter0();
    void testFieldGetterN();

    void testNonTrivialFieldGetter0();
    void testNonTrivialFieldGetterN();

    void testFieldSetter0();
    void fieldSetter0(const std::string& source);
    void testFieldSetterN();
    void fieldSetterN(const std::string& source);
    void testNonTrivialFieldSetter0();
    void testNonTrivialFieldSetterN();

    void testBlockReturn();
};
