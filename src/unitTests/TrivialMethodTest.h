#pragma once

#include <cppunit/extensions/HelperMacros.h>

#include "TestWithParsing.h"

class TrivialMethodTest : public TestWithParsing {
    CPPUNIT_TEST_SUITE(TrivialMethodTest);
    CPPUNIT_TEST(testLiteralReturn);
    CPPUNIT_TEST(testLiteralNoReturn);
    CPPUNIT_TEST(testBlockLiteralReturn);
    CPPUNIT_TEST(testNonTrivialLiteralReturn);
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
};
