#include "TaggingTests.h"

#include <cppunit/TestAssert.h>

#include "../vmobjects/ObjectFormats.h"

void TaggingTests::testIntsAndRanges(TagTestData data) {
    CPPUNIT_ASSERT_EQUAL(data.expectedResult,
                         VMTAGGED_INTEGER_WITHIN_RANGE_CHECK(data.value));
}
