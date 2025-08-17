#include "InfIntTests.h"

#include <cppunit/TestAssert.h>
#include <cstdint>

#include "../lib/InfInt.h"
#include "../vmobjects/ObjectFormats.h"

void InfIntTest::testBasicNumbers() {
    InfInt const zero(int64_t(0LL));
    CPPUNIT_ASSERT_EQUAL(int64_t(0LL), zero.toInt64());

    InfInt const one(int64_t(1LL));
    CPPUNIT_ASSERT_EQUAL(int64_t(1LL), one.toInt64());

    InfInt const a500(int64_t(500LL));
    CPPUNIT_ASSERT_EQUAL(int64_t(500LL), a500.toInt64());

    InfInt const a32bitNum(int64_t(3221258751LL));
    CPPUNIT_ASSERT_EQUAL(int64_t(3221258751LL), a32bitNum.toInt64());

    InfInt const a48bitNum(int64_t(211109453791743LL));
    CPPUNIT_ASSERT_EQUAL(int64_t(211109453791743LL), a48bitNum.toInt64());

    InfInt const a63bitNum(int64_t(8070661641701720575LL));
    CPPUNIT_ASSERT_EQUAL(int64_t(8070661641701720575LL), a63bitNum.toInt64());
}

void InfIntTest::testIsZero() {
    InfInt const zero{};
    CPPUNIT_ASSERT_EQUAL(int64_t(0LL), zero.toInt64());
    CPPUNIT_ASSERT(zero.isZero());

    InfInt const zeroInt64(int64_t(0LL));
    CPPUNIT_ASSERT_EQUAL(int64_t(0LL), zeroInt64.toInt64());
    CPPUNIT_ASSERT(zeroInt64.isZero());

    InfInt const zeroStr("0");
    CPPUNIT_ASSERT_EQUAL(int64_t(0LL), zeroStr.toInt64());
    CPPUNIT_ASSERT(zeroStr.isZero());

    InfInt const negZeroStr("-0");
    CPPUNIT_ASSERT_EQUAL(int64_t(0LL), negZeroStr.toInt64());
    CPPUNIT_ASSERT(negZeroStr.isZero());
}

void InfIntTest::testIsWithinSmallIntRange() {
    InfInt const smallIntMax(VMTAGGEDINTEGER_MAX);
    CPPUNIT_ASSERT_EQUAL(int64_t(VMTAGGEDINTEGER_MAX), smallIntMax.toInt64());
    CPPUNIT_ASSERT(smallIntMax.isWithinSmallIntRange());

    InfInt const smallIntMin(VMTAGGEDINTEGER_MIN);
    CPPUNIT_ASSERT_EQUAL(int64_t(VMTAGGEDINTEGER_MIN), smallIntMin.toInt64());
    CPPUNIT_ASSERT(smallIntMin.isWithinSmallIntRange());

    InfInt const smallIntMaxPlusOne(VMTAGGEDINTEGER_MAX + 1LL);
    CPPUNIT_ASSERT_EQUAL(int64_t(VMTAGGEDINTEGER_MAX + 1LL),
                         smallIntMaxPlusOne.toInt64());
    CPPUNIT_ASSERT(!smallIntMaxPlusOne.isWithinSmallIntRange());

    InfInt const smallIntMinMinusOne(VMTAGGEDINTEGER_MIN - 1LL);
    CPPUNIT_ASSERT_EQUAL(int64_t(VMTAGGEDINTEGER_MIN - 1LL),
                         smallIntMinMinusOne.toInt64());
    CPPUNIT_ASSERT(!smallIntMinMinusOne.isWithinSmallIntRange());

    InfInt const smallIntMaxMinusOne(VMTAGGEDINTEGER_MAX - 1LL);
    CPPUNIT_ASSERT_EQUAL(int64_t(VMTAGGEDINTEGER_MAX - 1LL),
                         smallIntMaxMinusOne.toInt64());
    CPPUNIT_ASSERT(smallIntMaxMinusOne.isWithinSmallIntRange());

    InfInt const smallIntMinPlusOne(VMTAGGEDINTEGER_MIN + 1LL);
    CPPUNIT_ASSERT_EQUAL(int64_t(VMTAGGEDINTEGER_MIN + 1LL),
                         smallIntMinPlusOne.toInt64());
    CPPUNIT_ASSERT(smallIntMinPlusOne.isWithinSmallIntRange());
}
