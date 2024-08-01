#pragma once

#include <cppunit/extensions/HelperMacros.h>

#include "../compiler/ClassGenerationContext.h"
#include "../compiler/MethodGenerationContext.h"
#include "../interpreter/bytecodes.h"

class BC {
public:
    BC(uint8_t bytecode) : bytecode(bytecode), arg1(0), arg2(0), size(1) {}

    BC(uint8_t bytecode, uint8_t arg1)
        : bytecode(bytecode), arg1(arg1), arg2(0), size(2) {}

    BC(uint8_t bytecode, uint8_t arg1, uint8_t arg2)
        : bytecode(bytecode), arg1(arg1), arg2(arg2), size(3) {}

    uint8_t bytecode;
    uint8_t arg1;
    uint8_t arg2;

    size_t size;
};

class BytecodeGenerationTest : public CPPUNIT_NS::TestCase {
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

    CPPUNIT_TEST(testWhileInliningWhileTrue);
    CPPUNIT_TEST(testWhileInliningWhileFalse);
    CPPUNIT_TEST(testInliningWhileLoopsWithExpandingBranches);
    CPPUNIT_TEST(testInliningWhileLoopsWithContractingBranches);
    CPPUNIT_TEST(testIfInlineAndConstantBcLength);
    CPPUNIT_TEST(testIfTrueWithLiteralReturn);
    CPPUNIT_TEST(testIfTrueWithSomethingAndLiteralReturn);

    CPPUNIT_TEST(testJumpQueuesOrdering);

    CPPUNIT_TEST_SUITE_END();

public:
    inline void setUp() {}

    inline void tearDown() {
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

    std::vector<uint8_t> methodToBytecode(const char* source,
                                          bool dumpBytecodes = false);
    std::vector<uint8_t> blockToBytecode(const char* source,
                                         bool dumpBytecodes = false);

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

    void testWhileInliningWhileTrue() {
        testWhileInlining("whileTrue:", BC_JUMP_ON_FALSE_POP);
    }

    void testWhileInliningWhileFalse() {
        testWhileInlining("whileFalse:", BC_JUMP_ON_TRUE_POP);
    }

    void testWhileInlining(const char* selector, uint8_t jumpBytecode);

    void testInliningWhileLoopsWithExpandingBranches();
    void testInliningWhileLoopsWithContractingBranches();
    void testIfInlineAndConstantBcLength();

    void testIfTrueWithLiteralReturn();
    void ifTrueWithLiteralReturn(std::string literal, BC bytecode);

    void testIfTrueWithSomethingAndLiteralReturn();
    void ifTrueWithSomethingAndLiteralReturn(std::string literal, BC bytecode);

    void testJumpQueuesOrdering();

    void dump(MethodGenerationContext* mgenc);

    void check(std::vector<uint8_t> actual, std::vector<BC> expected);
};
