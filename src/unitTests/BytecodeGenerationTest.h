#pragma once

#include <cppunit/extensions/HelperMacros.h>

#include <compiler/ClassGenerationContext.h>
#include <compiler/MethodGenerationContext.h>

class BytecodeGenerationTest: public CPPUNIT_NS::TestCase {

    CPPUNIT_TEST_SUITE (BytecodeGenerationTest);
    CPPUNIT_TEST (testEmptyMethodReturnsSelf);
    CPPUNIT_TEST (testPushConstant);
    CPPUNIT_TEST (testIfPushConstantSame);
    CPPUNIT_TEST (testIfPushConstantDifferent);
    CPPUNIT_TEST_SUITE_END();

public:
    inline void setUp(void) {
    }
    inline void tearDown(void) {
        delete _cgenc;
        delete _mgenc;
    }

private:
    ClassGenerationContext* _cgenc;
    MethodGenerationContext* _mgenc;
    
    ClassGenerationContext* makeCGenC();
    MethodGenerationContext* makeMGenC();
    
    std::vector<uint8_t> methodToBytecode(MethodGenerationContext* mgenc, const char* source, bool dumpBytecodes = false);
    
    void testEmptyMethodReturnsSelf();
    
    void testPushConstant();
    void testIfPushConstantSame();
    void testIfPushConstantDifferent();
    
    void dump(MethodGenerationContext* mgenc);
    
    void check(std::vector<uint8_t> actual, std::vector<uint8_t> expected);
};
