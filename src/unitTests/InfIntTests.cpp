#include "InfIntTests.h"

#include <cppunit/TestAssert.h>
#include <cstdint>

#include "../lib/InfInt.h"

void InfIntTest::testBasicNumbers() {
    InfInt const zero(int64_t(0LL));
    CPPUNIT_ASSERT_EQUAL(int64_t(0LL), zero.toLongLong());

    InfInt const one(int64_t(1LL));
    CPPUNIT_ASSERT_EQUAL(int64_t(1LL), one.toLongLong());

    InfInt const a500(int64_t(500LL));
    CPPUNIT_ASSERT_EQUAL(int64_t(500LL), a500.toLongLong());

    InfInt const a32bitNum(int64_t(3221258751LL));
    CPPUNIT_ASSERT_EQUAL(int64_t(3221258751LL), a32bitNum.toLongLong());

    InfInt const a48bitNum(int64_t(211109453791743LL));
    CPPUNIT_ASSERT_EQUAL(int64_t(211109453791743LL), a48bitNum.toLongLong());

    InfInt const a63bitNum(int64_t(8070661641701720575LL));
    CPPUNIT_ASSERT_EQUAL(int64_t(8070661641701720575LL),
                         a63bitNum.toLongLong());
}

void InfIntTest::testIsZero() {
    InfInt const zero{};
    CPPUNIT_ASSERT_EQUAL(int64_t(0LL), zero.toLongLong());
    CPPUNIT_ASSERT(zero.isZero());

    InfInt const zeroInt64(int64_t(0LL));
    CPPUNIT_ASSERT_EQUAL(int64_t(0LL), zeroInt64.toLongLong());
    CPPUNIT_ASSERT(zeroInt64.isZero());

    InfInt const zeroStr("0");
    CPPUNIT_ASSERT_EQUAL(int64_t(0LL), zeroStr.toLongLong());
    CPPUNIT_ASSERT(zeroStr.isZero());

    InfInt const negZeroStr("-0");
    CPPUNIT_ASSERT_EQUAL(int64_t(0LL), negZeroStr.toLongLong());
    CPPUNIT_ASSERT(negZeroStr.isZero());
}
