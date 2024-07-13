#pragma once

#include <cppunit/extensions/HelperMacros.h>

#include <compiler/ClassGenerationContext.h>
#include <compiler/MethodGenerationContext.h>

class BytecodeGenerationTest: public CPPUNIT_NS::TestCase {

    CPPUNIT_TEST_SUITE(BytecodeGenerationTest);
    CPPUNIT_TEST(testEmptyMethodReturnsSelf);
    CPPUNIT_TEST(testPushConstant);
    CPPUNIT_TEST(testIfPushConstantSame);
    CPPUNIT_TEST(testIfPushConstantDifferent);
    
    CPPUNIT_TEST(testExplicitReturnSelf);
    CPPUNIT_TEST(testDupPopArgumentPop);
    CPPUNIT_TEST(testDupPopArgumentPopImplicitReturnSelf);
    CPPUNIT_TEST(testDupPopLocalPop);
    CPPUNIT_TEST(testDupPopField0Pop);
    CPPUNIT_TEST(testDupPopFieldPop);
    CPPUNIT_TEST(testDupPopFieldReturnSelf);
    CPPUNIT_TEST(testDupPopFieldNReturnSelf);
    CPPUNIT_TEST(testSendDupPopFieldReturnLocal);
    CPPUNIT_TEST(testSendDupPopFieldReturnLocalPeriod);
    CPPUNIT_TEST(testBlockDupPopArgumentPopReturnArg);
    CPPUNIT_TEST(testBlockDupPopArgumentImplicitReturn);
    CPPUNIT_TEST(testBlockDupPopArgumentImplicitReturnDot);
    CPPUNIT_TEST(testBlockDupPopLocalReturnLocal);
    CPPUNIT_TEST(testBlockDupPopFieldReturnLocal);
    CPPUNIT_TEST(testBlockDupPopFieldReturnLocalDot);
    CPPUNIT_TEST(testPushLocalOpt);
    CPPUNIT_TEST(testPushArgOpt);
    CPPUNIT_TEST(testPushFieldOpt);
    CPPUNIT_TEST(testBlockPushFieldOpt);
    CPPUNIT_TEST(testPopLocalOpt);
    CPPUNIT_TEST(testPopFieldOpt);
    
    CPPUNIT_TEST_SUITE_END();

public:
    inline void setUp(void) {
    }
    
    inline void tearDown(void) {
        delete _cgenc;
        _cgenc = nullptr;
        
        delete _mgenc;
        _mgenc = nullptr;
        
        delete _bgenc;
        _bgenc = nullptr;
    }

private:
    ClassGenerationContext* _cgenc;
    MethodGenerationContext* _mgenc;
    MethodGenerationContext* _bgenc;
    
    void ensureCGenC();
    void ensureMGenC();
    void ensureBGenC();
    void addField(const char* fieldName);
    
    std::vector<uint8_t> methodToBytecode(const char* source, bool dumpBytecodes = false);
    std::vector<uint8_t> blockToBytecode(const char* source, bool dumpBytecodes = false);
    
    void testEmptyMethodReturnsSelf();
    
    void testPushConstant();
    void testIfPushConstantSame();
    void testIfPushConstantDifferent();
    
    void testExplicitReturnSelf();
    void testDupPopArgumentPop();
    void testDupPopArgumentPopImplicitReturnSelf();
    void testDupPopLocalPop();
    void testDupPopField0Pop();
    void testDupPopFieldPop();
    void testDupPopFieldReturnSelf();
    void testDupPopFieldNReturnSelf();
    void testSendDupPopFieldReturnLocal();
    void testSendDupPopFieldReturnLocalPeriod();
    
    void testBlockDupPopArgumentPopReturnArg();
    void testBlockDupPopArgumentImplicitReturn();
    void testBlockDupPopArgumentImplicitReturnDot();
    void testBlockDupPopLocalReturnLocal();
    void testBlockDupPopFieldReturnLocal();
    void testBlockDupPopFieldReturnLocalDot();
    
    void testPushLocalOpt();
    void testPushArgOpt();
    void testPushFieldOpt();
    void testBlockPushFieldOpt();
    void testPopLocalOpt();
    void testPopFieldOpt();
    
    void dump(MethodGenerationContext* mgenc);
    
    void check(std::vector<uint8_t> actual, std::vector<uint8_t> expected);
};
