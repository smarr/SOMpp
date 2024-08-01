#pragma once

#include <cppunit/extensions/HelperMacros.h>

class InlineCacheTest : public CPPUNIT_NS::TestCase {
    CPPUNIT_TEST_SUITE(InlineCacheTest);
    //    CPPUNIT_TEST(testCaching);
    CPPUNIT_TEST_SUITE_END();

public:
    inline void setUp() {}

    inline void tearDown() {}

private:
    void testCaching();

    void cachingAtBytecodeIndex(size_t bytecodeIndex);
};
