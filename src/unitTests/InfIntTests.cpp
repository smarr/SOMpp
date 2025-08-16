#include "InfIntTests.h"

#include <cppunit/TestAssert.h>

#include "../lib/InfInt.h"

void InfIntTest::testBasicNumbers() {
    InfInt const zero(0LL);
    CPPUNIT_ASSERT_EQUAL(0LL, zero.toLongLong());

    InfInt const one(1LL);
    CPPUNIT_ASSERT_EQUAL(1LL, one.toLongLong());

    InfInt const a500(500LL);
    CPPUNIT_ASSERT_EQUAL(500LL, a500.toLongLong());

    InfInt const a32bitNum(3221258751LL);
    CPPUNIT_ASSERT_EQUAL(3221258751LL, a32bitNum.toLongLong());

    InfInt const a48bitNum(211109453791743LL);
    CPPUNIT_ASSERT_EQUAL(211109453791743LL, a48bitNum.toLongLong());

    InfInt const a63bitNum(8070661641701720575LL);
    CPPUNIT_ASSERT_EQUAL(8070661641701720575LL, a63bitNum.toLongLong());
}
