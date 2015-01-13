#pragma once

#include <vmobjects/VMArray.h>
#include <cppunit/extensions/HelperMacros.h>
#include <memory/Heap.h>

using namespace std;

class WriteBarrierTest: public CPPUNIT_NS::TestCase {

    CPPUNIT_TEST_SUITE (WriteBarrierTest);
    CPPUNIT_TEST (testWriteArray);
    CPPUNIT_TEST (testWriteClass);
    CPPUNIT_TEST (testWriteBlock);
    CPPUNIT_TEST (testWriteFrame);
    CPPUNIT_TEST (testWriteEvaluationPrimitive);
    CPPUNIT_TEST (testWriteMethod);CPPUNIT_TEST_SUITE_END();

public:
    inline void setUp(void) {
        Heap<HEAP_CLS>::InitializeHeap(512, 1024*1024);
    }
    inline void tearDown(void) {
    }

private:
    void testWriteArray();
    void testWriteClass();
    void testWriteBlock();
    void testWriteFrame();
    void testWriteMethod();
    void testWriteEvaluationPrimitive();

};
