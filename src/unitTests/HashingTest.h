#pragma once

#include <cppunit/extensions/HelperMacros.h>

#include "TestWithParsing.h"

class HashingTest : public TestWithParsing {
    CPPUNIT_TEST_SUITE(HashingTest);
    CPPUNIT_TEST(testMurmur3HashWithSeeds);
    CPPUNIT_TEST_SUITE_END();

private:
    static void testMurmur3HashWithSeeds();
    static void testMurmur3HashWithSeed(const void* key, size_t len, uint32_t seed, uint32_t expectedHash);

};