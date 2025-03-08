#pragma once

#include <cppunit/extensions/HelperMacros.h>

using namespace std;

class TagTestData {
public:
    int64_t value;
    bool expectedResult;

    TagTestData(int64_t value, bool expectedResult)
        : value(value), expectedResult(expectedResult) {}
};

inline std::ostream& operator<<(std::ostream& strm, const TagTestData& data) {
    strm << data.value << " expected: " << data.expectedResult;
    return strm;
}

class TaggingTests : public CPPUNIT_NS::TestCase {
    CPPUNIT_TEST_SUITE(TaggingTests);  // NOLINT(misc-const-correctness)
    CPPUNIT_TEST_PARAMETERIZED(
        testIntsAndRanges,
        {TagTestData(0x3FFF'FFFF'FFFF'FFFFLL, true),
         TagTestData(0x3FFF'FFFF'FFFF'FFFFLL + 1LL, false),
         TagTestData(0x3FFF'FFFF'FFFF'FFFFLL - 1LL, true),

         TagTestData(-0x4000'0000'0000'0000LL, true),
         TagTestData(-0x4000'0000'0000'0000LL - 1LL, false),
         TagTestData(-0x4000'0000'0000'0000LL + 1LL, true),

         // NOLINTNEXTLINE(hicpp-signed-bitwise)
         TagTestData(1LL << 63, false), TagTestData(1LL << 62, false),
         TagTestData(1LL, true), TagTestData(-1LL, true)});
    CPPUNIT_TEST_SUITE_END();

public:
    inline void setUp() override {}
    inline void tearDown() override {}

private:
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void testIntsAndRanges(TagTestData data);
};
