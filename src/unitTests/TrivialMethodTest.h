#pragma once

#include <cppunit/extensions/HelperMacros.h>

#include "TestWithParsing.h"

class TrivialMethodTest : public TestWithParsing {
    CPPUNIT_TEST_SUITE(TrivialMethodTest);
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
    CPPUNIT_TEST_SUITE_END();

private:
    void testLiteralReturn();
    void literalReturn(std::string source);

    void testBlockLiteralReturn();
    void blockLiteralReturn(std::string source);

    void testLiteralNoReturn();
    void literalNoReturn(std::string source);

    void testNonTrivialLiteralReturn();
    void nonTrivialLiteralReturn(std::string source);

    void testGlobalReturn();
    void globalReturn(std::string source);

    void testNonTrivialGlobalReturn();
    void testUnknownGlobalInBlock();

    void testFieldGetter0();
    void testFieldGetterN();

    void testNonTrivialFieldGetter0();
    void testNonTrivialFieldGetterN();
};
