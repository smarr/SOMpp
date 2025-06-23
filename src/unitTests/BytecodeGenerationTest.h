#pragma once

#include <cppunit/extensions/HelperMacros.h>

#include "../compiler/ClassGenerationContext.h"
#include "../compiler/MethodGenerationContext.h"
#include "../interpreter/bytecodes.h"
#include "TestWithParsing.h"
#include <cppunit/TestAssert.h>

class BytecodeGenerationTest : public TestWithParsing {
    // NOLINTNEXTLINE(misc-const-correctness)
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
    CPPUNIT_TEST(testIfTrueIfFalseArg);
    CPPUNIT_TEST(testIfTrueIfFalseNlrArg1);
    CPPUNIT_TEST(testIfTrueIfFalseNlrArg2);
    CPPUNIT_TEST(testInliningOfOr);
    CPPUNIT_TEST(testInliningOfAnd);

    CPPUNIT_TEST(testInliningOfToDo);
    CPPUNIT_TEST(testToDoBlockBlockInlinedSelf);
    CPPUNIT_TEST(testToDoWithMoreEmbeddedBlocksAndArgAccess);

    CPPUNIT_TEST(testIfArg);
    CPPUNIT_TEST(testKeywordIfTrueArg);
    CPPUNIT_TEST(testIfReturnNonLocal);

    CPPUNIT_TEST(testJumpQueuesOrdering);
    CPPUNIT_TEST(testNestedIfs);
    CPPUNIT_TEST(testNestedIfsAndLocals);

    CPPUNIT_TEST(testIncDecBytecodes);
    CPPUNIT_TEST(testIfTrueAndIncField);
    CPPUNIT_TEST(testIfTrueAndIncArg);

    CPPUNIT_TEST(testFieldReadInlining);
    CPPUNIT_TEST(testReturnField);
    CPPUNIT_TEST(testTrivialMethodInlining);
    CPPUNIT_TEST(testBlockIfTrueArg);
    CPPUNIT_TEST(testBlockIfTrueMethodArg);
    CPPUNIT_TEST(testIfTrueIfFalseReturn);
    CPPUNIT_TEST(testBlockIfReturnNonLocal);
    CPPUNIT_TEST(testIncField);
    CPPUNIT_TEST(testIncFieldNonTrivial);
    CPPUNIT_TEST(testReturnIncFieldFromBlock);
    CPPUNIT_TEST(testReturnIncField);
    CPPUNIT_TEST(testFieldReadIncWrite);

    CPPUNIT_TEST_SUITE_END();

private:
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

    void testIfTrueIfFalseArg();
    void testIfTrueIfFalseNlrArg1();
    void testIfTrueIfFalseNlrArg2();

    void testIfArg();
    void ifArg(std::string selector, int8_t jumpBytecode);
    void testKeywordIfTrueArg();

    void testIfReturnNonLocal();
    void ifReturnNonLocal(std::string selector, int8_t jumpBytecode);

    void testInliningOfOr();
    void inliningOfOr(std::string selector);
    void testInliningOfAnd();
    void inliningOfAnd(std::string selector);

    void testInliningOfToDo();
    void testToDoBlockBlockInlinedSelf();
    void testToDoWithMoreEmbeddedBlocksAndArgAccess();

    static void testJumpQueuesOrdering();

    void testNestedIfs();
    void testNestedIfsAndLocals();

    void testIncDecBytecodes();
    void incDecBytecodes(const std::string& sel, uint8_t bc);

    void testIfTrueAndIncField();
    void testIfTrueAndIncArg();

    void testFieldReadInlining();
    void testReturnField();
    void returnField(size_t fieldNum, BC bytecode, bool isReturnFieldBc);

    void testTrivialMethodInlining();
    void trivialMethodInlining(const std::string& literal, BC bytecode);

    void testBlockIfTrueArg();
    void testBlockIfTrueMethodArg();
    void testIfTrueIfFalseReturn();
    void ifTrueIfFalseReturn(const std::string& sel1, const std::string& sel2,
                             BC bc);

    void testBlockIfReturnNonLocal();
    void blockIfReturnNonLocal(std::string sel, BC bc);

    void testIncField();
    void incField(size_t fieldNum);

    void testIncFieldNonTrivial();
    void incFieldNonTrivial(size_t fieldNum);

    void testReturnIncField();
    void returnIncField(size_t fieldNum);

    void testReturnIncFieldFromBlock();
    void returnIncFieldFromBlock(size_t fieldNum);

    void testFieldReadIncWrite();
};
