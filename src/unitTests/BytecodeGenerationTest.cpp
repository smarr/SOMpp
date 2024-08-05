#include "BytecodeGenerationTest.h"

#include <cassert>
#include <cppunit/TestAssert.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

#include "../compiler/ClassGenerationContext.h"
#include "../compiler/Disassembler.h"
#include "../compiler/MethodGenerationContext.h"
#include "../compiler/Parser.h"
#include "../interpreter/bytecodes.h"
#include "../misc/StringUtil.h"
#include "../vm/Symbols.h"
#include "../vmobjects/VMMethod.h"

void BytecodeGenerationTest::dump(MethodGenerationContext* mgenc) {
    Disassembler::DumpMethod(mgenc, "");
}

void BytecodeGenerationTest::ensureCGenC() {
    if (_cgenc != nullptr) {
        return;
    }

    _cgenc = new ClassGenerationContext();
    _cgenc->SetName(SymbolFor("Test"));
}

void BytecodeGenerationTest::ensureMGenC() {
    if (_mgenc != nullptr) {
        return;
    }
    ensureCGenC();

    _mgenc = new MethodGenerationContext(*_cgenc);
    std::string self = strSelf;
    _mgenc->AddArgument(self, {0, 0});
}

void BytecodeGenerationTest::ensureBGenC() {
    if (_bgenc != nullptr) {
        return;
    }
    ensureCGenC();
    ensureMGenC();

    _mgenc->SetSignature(SymbolFor("test"));
    _bgenc = new MethodGenerationContext(*_cgenc, _mgenc);
}

void BytecodeGenerationTest::addField(const char* fieldName) {
    ensureCGenC();
    _cgenc->AddInstanceField(SymbolFor(fieldName));
}

std::vector<uint8_t> BytecodeGenerationTest::methodToBytecode(
    const char* source, bool dumpBytecodes) {
    ensureMGenC();

    istringstream ss(source);

    std::string fileName = "test";
    Parser parser(ss, fileName);
    parser.method(*_mgenc);

    if (dumpBytecodes) {
        dump(_mgenc);
    }
    return _mgenc->GetBytecodes();
}

std::vector<uint8_t> BytecodeGenerationTest::blockToBytecode(
    const char* source, bool dumpBytecodes) {
    ensureBGenC();

    istringstream ss(source);

    std::string fileName = "test";
    Parser parser(ss, fileName);

    parser.nestedBlock(*_bgenc);

    if (dumpBytecodes) {
        dump(_bgenc);
    }
    return _bgenc->GetBytecodes();
}

void BytecodeGenerationTest::testEmptyMethodReturnsSelf() {
    auto bytecodes = methodToBytecode("test = ( )");

    check(bytecodes, {BC_RETURN_SELF});
}

void BytecodeGenerationTest::testPushConstant() {
    auto bytecodes = methodToBytecode(R"""(
                                      test = (
                                        0. 1. nil. #a. true. false.
                                      ) )""");
    check(bytecodes,
          {BC_PUSH_0, BC_POP, BC_PUSH_1, BC_POP, BC_PUSH_NIL, BC_POP,
           BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_CONSTANT_1, BC_POP,
           BC_PUSH_CONSTANT_2, BC_POP, BC_RETURN_SELF});
}

void BytecodeGenerationTest::testIfPushConstantSame() {
    auto bytecodes = methodToBytecode(R"""(
                                      test = (
                                        #a. #b. #c. #d.
                                        true ifFalse: [ #a. #b. #c. #d. ]
                                      ) )""");
    check(bytecodes, {BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_CONSTANT_1, BC_POP,
                      BC_PUSH_CONSTANT_2, BC_POP, BC(BC_PUSH_CONSTANT, 3),
                      BC_POP, BC(BC_PUSH_CONSTANT, 4),
                      BC(BC_JUMP_ON_TRUE_TOP_NIL, 11, 0), BC_PUSH_CONSTANT_0,
                      BC_POP, BC_PUSH_CONSTANT_1, BC_POP, BC_PUSH_CONSTANT_2,
                      BC_POP, BC(BC_PUSH_CONSTANT, 3), BC_RETURN_SELF});
}

void BytecodeGenerationTest::testIfPushConstantDifferent() {
    auto bytecodes = methodToBytecode(R"""(
                                      test = (
                                        #a. #b. #c. #d.
                                        true ifFalse: [ #e. #f. #g. #h. ]
                                      ) )""");
    check(bytecodes,
          {BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_CONSTANT_1, BC_POP,
           BC_PUSH_CONSTANT_2, BC_POP, BC(BC_PUSH_CONSTANT, 3), BC_POP,
           BC(BC_PUSH_CONSTANT, 4), BC(BC_JUMP_ON_TRUE_TOP_NIL, 14, 0),
           BC(BC_PUSH_CONSTANT, 5), BC_POP, BC(BC_PUSH_CONSTANT, 6), BC_POP,
           BC(BC_PUSH_CONSTANT, 7), BC_POP, BC(BC_PUSH_CONSTANT, 8),
           BC_RETURN_SELF});
}

void BytecodeGenerationTest::testExplicitReturnSelf() {
    auto bytecodes = methodToBytecode("test = ( ^ self )");

    check(bytecodes, {BC_RETURN_SELF});
}

void BytecodeGenerationTest::testDupPopArgumentPop() {
    auto bytecodes = methodToBytecode("test: arg = ( arg := 1. ^ self )");

    check(bytecodes, {BC_PUSH_1, BC(BC_POP_ARGUMENT, 1, 0), BC_RETURN_SELF});
}

void BytecodeGenerationTest::testDupPopArgumentPopImplicitReturnSelf() {
    auto bytecodes = methodToBytecode("test: arg = ( arg := 1 )");

    check(bytecodes, {BC_PUSH_1, BC(BC_POP_ARGUMENT, 1, 0), BC_RETURN_SELF});
}

void BytecodeGenerationTest::testDupPopLocalPop() {
    auto bytecodes =
        methodToBytecode("test = ( | local | local := 1. ^ self )");

    check(bytecodes, {BC_PUSH_1, BC_POP_LOCAL_0, BC_RETURN_SELF});
}

void BytecodeGenerationTest::testDupPopField0Pop() {
    addField("field");
    auto bytecodes = methodToBytecode("test = ( field := 1. ^ self )");

    check(bytecodes, {BC_PUSH_1, BC_POP_FIELD_0, BC_RETURN_SELF});
}

void BytecodeGenerationTest::testDupPopFieldPop() {
    addField("a");
    addField("b");
    addField("c");
    addField("d");
    addField("field");
    auto bytecodes = methodToBytecode("test = ( field := 1. ^ self )");

    check(bytecodes, {BC_PUSH_1, BC(BC_POP_FIELD, 4), BC_RETURN_SELF});
}

void BytecodeGenerationTest::testDupPopFieldReturnSelf() {
    addField("field");
    auto bytecodes = methodToBytecode("test: val = ( field := val )");

    check(bytecodes, {BC_PUSH_ARG_1, BC_POP_FIELD_0, BC_RETURN_SELF});
}

void BytecodeGenerationTest::testDupPopFieldNReturnSelf() {
    addField("a");
    addField("b");
    addField("c");
    addField("d");
    addField("e");
    addField("field");
    auto bytecodes = methodToBytecode("test: val = ( field := val )");

    check(bytecodes, {BC_PUSH_ARG_1, BC(BC_POP_FIELD, 5), BC_RETURN_SELF});
}

void BytecodeGenerationTest::testSendDupPopFieldReturnLocal() {
    addField("field");
    auto bytecodes = methodToBytecode("test = ( ^ field := self method )");

    check(bytecodes,
          {BC_PUSH_SELF, BC(BC_SEND, 0), BC_DUP, BC_POP_FIELD_0,
           BC_RETURN_LOCAL});
}

void BytecodeGenerationTest::testSendDupPopFieldReturnLocalPeriod() {
    addField("field");
    auto bytecodes = methodToBytecode("test = ( ^ field := self method. )");

    check(bytecodes,
          {BC_PUSH_SELF, BC(BC_SEND, 0), BC_DUP, BC_POP_FIELD_0,
           BC_RETURN_LOCAL});
}

void BytecodeGenerationTest::testBlockDupPopArgumentPopReturnArg() {
    auto bytecodes = blockToBytecode("[:arg | arg := 1. arg ]");

    check(bytecodes, {BC_PUSH_1, BC(BC_POP_ARGUMENT, 1, 0), BC_PUSH_ARG_1,
                      BC_RETURN_LOCAL});
}

void BytecodeGenerationTest::testBlockDupPopArgumentImplicitReturn() {
    auto bytecodes = blockToBytecode("[:arg | arg := 1 ]");
    check(bytecodes,
          {BC_PUSH_1, BC_DUP, BC(BC_POP_ARGUMENT, 1, 0), BC_RETURN_LOCAL});
}

void BytecodeGenerationTest::testBlockDupPopArgumentImplicitReturnDot() {
    auto bytecodes = blockToBytecode("[:arg | arg := 1. ]");
    check(bytecodes,
          {BC_PUSH_1, BC_DUP, BC(BC_POP_ARGUMENT, 1, 0), BC_RETURN_LOCAL});
}

void BytecodeGenerationTest::testBlockDupPopLocalReturnLocal() {
    auto bytecodes = blockToBytecode("[| local | local := 1 ]");
    check(bytecodes, {BC_PUSH_1, BC_DUP, BC_POP_LOCAL_0, BC_RETURN_LOCAL});
}

void BytecodeGenerationTest::testBlockDupPopFieldReturnLocal() {
    addField("field");
    auto bytecodes = blockToBytecode("[ field := 1 ]");
    check(bytecodes, {BC_PUSH_1, BC_DUP, BC_POP_FIELD_0, BC_RETURN_LOCAL});
}

void BytecodeGenerationTest::testBlockDupPopFieldReturnLocalDot() {
    addField("field");
    auto bytecodes = blockToBytecode("[ field := 1 ]");
    check(bytecodes, {BC_PUSH_1, BC_DUP, BC_POP_FIELD_0, BC_RETURN_LOCAL});
}

void BytecodeGenerationTest::testPushLocalOpt() {
    auto bytecodes = methodToBytecode(R"""(
                                      test = (
                                        | l1 l2 l3 l4 |
                                        l1. l2. l3. l4.
                                      ) )""");
    check(bytecodes,
          {BC_PUSH_LOCAL_0, BC_POP, BC_PUSH_LOCAL_1, BC_POP, BC_PUSH_LOCAL_2,
           BC_POP, BC(BC_PUSH_LOCAL, 3, 0), BC_POP, BC_RETURN_SELF});
}

void BytecodeGenerationTest::testPushArgOpt() {
    auto bytecodes = methodToBytecode(R"""(
                                      test: a1 and: a2 and: a3 and: a4 = (
                                        self. a1. a2. a3. a4.
                                      ) )""");
    check(bytecodes,
          {BC_PUSH_SELF, BC_POP, BC_PUSH_ARG_1, BC_POP, BC_PUSH_ARG_2, BC_POP,
           BC(BC_PUSH_ARGUMENT, 3, 0), BC_POP, BC(BC_PUSH_ARGUMENT, 4, 0),
           BC_POP, BC_RETURN_SELF});
}

void BytecodeGenerationTest::testPushFieldOpt() {
    addField("f1");
    addField("f2");
    addField("f3");
    addField("f4");
    auto bytecodes = methodToBytecode("test = ( f1. f2. f3. f4 )");
    check(bytecodes,
          {BC_PUSH_FIELD_0, BC_POP, BC_PUSH_FIELD_1, BC_POP,
           BC(BC_PUSH_FIELD, 2), BC_POP, BC(BC_PUSH_FIELD, 3), BC_RETURN_SELF});
}

void BytecodeGenerationTest::testBlockPushFieldOpt() {
    addField("f1");
    addField("f2");
    addField("f3");
    addField("f4");
    auto bytecodes = blockToBytecode("[ f1. f2. f3. f4 ]");

    check(
        bytecodes,
        {BC_PUSH_FIELD_0, BC_POP, BC_PUSH_FIELD_1, BC_POP, BC(BC_PUSH_FIELD, 2),
         BC_POP, BC(BC_PUSH_FIELD, 3), BC_RETURN_LOCAL});
}

void BytecodeGenerationTest::testPopLocalOpt() {
    auto bytecodes = methodToBytecode(R"""(
                                      test = (
                                        | l1 l2 l3 l4 l5 |
                                        l1 := 1.
                                        l2 := 1.
                                        l3 := 1.
                                        l4 := 1.
                                        l5 := 1.
                                      ) )""");
    check(bytecodes,
          {BC_PUSH_1, BC_POP_LOCAL_0, BC_PUSH_1, BC_POP_LOCAL_1, BC_PUSH_1,
           BC_POP_LOCAL_2, BC_PUSH_1, BC(BC_POP_LOCAL, 3, 0), BC_PUSH_1,
           BC(BC_POP_LOCAL, 4, 0), BC_RETURN_SELF});
}

void BytecodeGenerationTest::testPopFieldOpt() {
    addField("f1");
    addField("f2");
    addField("f3");
    addField("f4");
    auto bytecodes = methodToBytecode(R"""(
                                      test = (
                                        f1 := 1.
                                        f2 := 1.
                                        f3 := 1.
                                        f4 := 1.
                                      ) )""");
    check(
        bytecodes,
        {BC_PUSH_1, BC_POP_FIELD_0, BC_PUSH_1, BC_POP_FIELD_1, BC_PUSH_1,
         BC(BC_POP_FIELD, 2), BC_PUSH_1, BC(BC_POP_FIELD, 3), BC_RETURN_SELF});
}

void BytecodeGenerationTest::check(std::vector<uint8_t> actual,
                                   std::vector<BC>
                                       expected) {
    size_t i = 0;
    size_t bci = 0;
    for (; bci < actual.size() && i < expected.size();) {
        uint8_t actualBc = actual.at(bci);
        uint8_t bcLength = Bytecode::GetBytecodeLength(actualBc);

        BC expectedBc = expected.at(i);

        char msg[1000];
        snprintf(msg, 1000, "Bytecode %zu expected %s but got %s", i,
                 Bytecode::GetBytecodeName(expectedBc.bytecode),
                 Bytecode::GetBytecodeName(actualBc));
        if (expectedBc.bytecode != actualBc) {
            dump(_mgenc);
        }
        CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, expectedBc.bytecode, actualBc);

        snprintf(
            msg, 1000,
            "Bytecode %zu (%s) was expected to have length %zu, but had %zu", i,
            Bytecode::GetBytecodeName(actualBc), expectedBc.size,
            (size_t)bcLength);

        if (expectedBc.size != bcLength) {
            dump(_mgenc);
        }
        CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, expectedBc.size, (size_t)bcLength);

        if (bcLength > 1) {
            snprintf(msg, 1000,
                     "Bytecode %zu (%s), arg1 expected %hhu but got %hhu", i,
                     Bytecode::GetBytecodeName(expectedBc.bytecode),
                     expectedBc.arg1, actual.at(bci + 1));
            if (expectedBc.arg1 != actual.at(bci + 1)) {
                dump(_mgenc);
            }
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, expectedBc.arg1,
                                         actual.at(bci + 1));

            if (bcLength > 2) {
                snprintf(msg, 1000,
                         "Bytecode %zu (%s), arg2 expected %hhu but got %hhu",
                         i, Bytecode::GetBytecodeName(expectedBc.bytecode),
                         expectedBc.arg2, actual.at(bci + 2));
                if (expectedBc.arg2 != actual.at(bci + 2)) {
                    dump(_mgenc);
                }
                CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, expectedBc.arg2,
                                             actual.at(bci + 2));
            }
        }

        i += 1;
        bci += bcLength;
    }
    if (expected.size() != i || actual.size() != bci) {
        dump(_mgenc);
    }

    CPPUNIT_ASSERT_EQUAL_MESSAGE("All expected bytecodes covered",
                                 expected.size(), i);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("All actual bytecodes covered", actual.size(),
                                 bci);
}

void BytecodeGenerationTest::testWhileInlining(const char* selector,
                                               uint8_t jumpBytecode) {
    std::string source = R"""(   test: arg = (
                                      #start.
                                      [ true ] SELECTOR [ arg ].
                                      #end
                                  ) )""";
    bool wasReplaced = ReplacePattern(source, "SELECTOR", selector);
    assert(wasReplaced);

    auto bytecodes = methodToBytecode(source.data());
    check(
        bytecodes,
        {BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_CONSTANT_1, BC(jumpBytecode, 8, 0),
         BC_PUSH_ARG_1, BC_POP, BC(BC_JUMP_BACKWARD, 6, 0), BC_PUSH_NIL, BC_POP,
         BC_PUSH_CONSTANT_2, BC_RETURN_SELF});
}

/** This test checks whether the jumps in the while loop are correct after it
 * got inlined. */
void BytecodeGenerationTest::testInliningWhileLoopsWithExpandingBranches() {
    auto bytecodes = methodToBytecode(R"""(
        test = (
          #const0. #const1. #const2.
          0 ifTrue: [
            [ #const3. #const4. #const5 ]
               whileTrue: [
                 #const6. #const7. #const8 ]
          ].
          ^ #end
        )  )""");

    check(bytecodes,
          {BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_CONSTANT_1, BC_POP,
           BC_PUSH_CONSTANT_2, BC_POP, BC_PUSH_0,

           // jump offset, to jump to the pop BC after the if/right before the
           // push #end
           BC(BC_JUMP_ON_FALSE_TOP_NIL, 27, 0), BC(BC_PUSH_CONSTANT, 3), BC_POP,
           BC(BC_PUSH_CONSTANT, 4), BC_POP, BC(BC_PUSH_CONSTANT, 5),

           // jump offset, jump to push_nil as result of whileTrue
           BC(BC_JUMP_ON_FALSE_POP, 15, 0), BC(BC_PUSH_CONSTANT, 6), BC_POP,
           BC(BC_PUSH_CONSTANT, 7), BC_POP, BC(BC_PUSH_CONSTANT, 8), BC_POP,

           // jump offset, jump back to the first PUSH_CONST inside if body,
           // pushing #const3
           BC(BC_JUMP_BACKWARD, 20, 0), BC_PUSH_NIL, BC_POP,

           BC(BC_PUSH_CONSTANT, 9), BC_RETURN_LOCAL});
}

void BytecodeGenerationTest::testInliningWhileLoopsWithContractingBranches() {
    auto bytecodes = methodToBytecode(R"""(
                        test = (
                        0 ifTrue: [
                            [ ^ 1 ]
                                whileTrue: [
                                ^ 0 ]
                        ].
                        ^ #end
                        ) )""");

    check(bytecodes,
          {BC_PUSH_0,
           // jump offset to jump to the pop after the if, before pushing #end
           BC(BC_JUMP_ON_FALSE_TOP_NIL, 15, 0), BC_PUSH_1, BC_RETURN_LOCAL,
           // jump offset, jump to push_nil as result of whileTrue
           BC(BC_JUMP_ON_FALSE_POP, 9, 0), BC_PUSH_0, BC_RETURN_LOCAL, BC_POP,
           // jump offset, to the push_1 of the condition
           BC(BC_JUMP_BACKWARD, 8, 0), BC_PUSH_NIL, BC_POP, BC_PUSH_CONSTANT_0,
           BC_RETURN_LOCAL});
};

void BytecodeGenerationTest::testIfInlineAndConstantBcLength() {
    auto bytecodes = methodToBytecode(R"""(
                                test = (
                                  #a. #b. #c.
                                  true ifTrue: [
                                    true ifFalse: [ #e. #f. #g ] ]
                                ) )""");

    CPPUNIT_ASSERT_EQUAL((uint8_t)BC_JUMP_ON_TRUE_TOP_NIL, bytecodes.at(13));

    CPPUNIT_ASSERT_EQUAL_MESSAGE(
        "jump offset, should point to correct bytecode and not be affected by "
        "changing length of bytecodes in the block",
        (uint8_t)11, bytecodes.at(14));
}

void BytecodeGenerationTest::testIfTrueWithLiteralReturn() {
    ifTrueWithLiteralReturn("0", BC_PUSH_0);
    ifTrueWithLiteralReturn("1", BC_PUSH_1);
    ifTrueWithLiteralReturn("-10", BC_PUSH_CONSTANT_1);
    ifTrueWithLiteralReturn("3333", BC_PUSH_CONSTANT_1);
    ifTrueWithLiteralReturn("'str'", BC_PUSH_CONSTANT_1);
    ifTrueWithLiteralReturn("#sym", BC_PUSH_CONSTANT_1);
    ifTrueWithLiteralReturn("1.1", BC_PUSH_CONSTANT_1);
    ifTrueWithLiteralReturn("-2342.234", BC_PUSH_CONSTANT_1);
    ifTrueWithLiteralReturn("true", BC_PUSH_CONSTANT_1);
    ifTrueWithLiteralReturn("false", BC_PUSH_CONSTANT_1);
    ifTrueWithLiteralReturn("nil", BC_PUSH_NIL);
    ifTrueWithLiteralReturn("SomeGlobal", BC(BC_PUSH_GLOBAL, 1));
    ifTrueWithLiteralReturn("[]", BC(BC_PUSH_BLOCK, 1));
    ifTrueWithLiteralReturn("[ self ]", BC(BC_PUSH_BLOCK, 1));
}

void BytecodeGenerationTest::ifTrueWithLiteralReturn(std::string literal,
                                                     BC bytecode) {
    std::string source = R"""(      test = (
                                        self method ifTrue: [ LITERAL ].
                                    ) )""";
    bool wasReplaced = ReplacePattern(source, "LITERAL", literal);
    assert(wasReplaced);

    auto bytecodes = methodToBytecode(source.data());

    bool twoByte2 = bytecode.bytecode == BC_PUSH_GLOBAL ||
                    bytecode.bytecode == BC_PUSH_BLOCK;

    check(bytecodes,
          {BC_PUSH_SELF, BC(BC_SEND, 0),
           BC(BC_JUMP_ON_FALSE_TOP_NIL, twoByte2 ? 5 : 4, 0), bytecode, BC_POP,
           BC_RETURN_SELF});

    tearDown();
}

void BytecodeGenerationTest::testIfTrueWithSomethingAndLiteralReturn() {
    ifTrueWithSomethingAndLiteralReturn("0", BC_PUSH_0);
    ifTrueWithSomethingAndLiteralReturn("1", BC_PUSH_1);
    ifTrueWithSomethingAndLiteralReturn("-10", BC_PUSH_CONSTANT_2);
    ifTrueWithSomethingAndLiteralReturn("3333", BC_PUSH_CONSTANT_2);
    ifTrueWithSomethingAndLiteralReturn("'str'", BC_PUSH_CONSTANT_2);
    ifTrueWithSomethingAndLiteralReturn("#sym", BC_PUSH_CONSTANT_2);
    ifTrueWithSomethingAndLiteralReturn("1.1", BC_PUSH_CONSTANT_2);
    ifTrueWithSomethingAndLiteralReturn("-2342.234", BC_PUSH_CONSTANT_2);
    ifTrueWithSomethingAndLiteralReturn("true", BC_PUSH_CONSTANT_2);
    ifTrueWithSomethingAndLiteralReturn("false", BC_PUSH_CONSTANT_2);
    ifTrueWithSomethingAndLiteralReturn("nil", BC_PUSH_NIL);
    ifTrueWithSomethingAndLiteralReturn("SomeGlobal", BC(BC_PUSH_GLOBAL, 2));
    ifTrueWithSomethingAndLiteralReturn("[]", BC(BC_PUSH_BLOCK, 2));
    ifTrueWithSomethingAndLiteralReturn("[ self ]", BC(BC_PUSH_BLOCK, 2));
}

void BytecodeGenerationTest::ifTrueWithSomethingAndLiteralReturn(
    std::string literal, BC bytecode) {
    // This test is different from the previous one, because the block
    // method won't be recognized as being trivial

    std::string source = R"""(      test = (
                                        self method ifTrue: [ #fooBarNonTrivialBlock. LITERAL ].
                                    ) )""";
    bool wasReplaced = ReplacePattern(source, "LITERAL", literal);
    assert(wasReplaced);

    auto bytecodes = methodToBytecode(source.data());

    bool twoByte2 = bytecode.bytecode == BC_PUSH_GLOBAL ||
                    bytecode.bytecode == BC_PUSH_BLOCK;

    check(bytecodes,
          {BC_PUSH_SELF, BC(BC_SEND, 0),
           BC(BC_JUMP_ON_FALSE_TOP_NIL, twoByte2 ? 7 : 6, 0),
           BC_PUSH_CONSTANT_1, BC_POP, bytecode, BC_POP, BC_RETURN_SELF});

    tearDown();
}

void BytecodeGenerationTest::testIfTrueIfFalseArg() {
    auto bytecodes = methodToBytecode(R"""( test: arg1 with: arg2 = (
            #start.
            self method ifTrue: [ arg1 ] ifFalse: [ arg2 ].
            #end
        ) )""");

    check(bytecodes,
          {BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_SELF, BC(BC_SEND, 1),
           BC(BC_JUMP_ON_FALSE_POP, 7, 0), BC_PUSH_ARG_1, BC(BC_JUMP, 4, 0),
           BC_PUSH_ARG_2, BC_POP, BC_PUSH_CONSTANT_2, BC_RETURN_SELF});
}

void BytecodeGenerationTest::testIfTrueIfFalseNlrArg1() {
    auto bytecodes = methodToBytecode(R"""( test: arg1 with: arg2 = (
            #start.
            self method ifTrue: [ ^ arg1 ] ifFalse: [ arg2 ].
            #end
        ) )""");

    check(bytecodes,
          {BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_SELF, BC(BC_SEND, 1),
           BC(BC_JUMP_ON_FALSE_POP, 8, 0), BC_PUSH_ARG_1, BC_RETURN_LOCAL,
           BC(BC_JUMP, 4, 0), BC_PUSH_ARG_2, BC_POP, BC_PUSH_CONSTANT_2,
           BC_RETURN_SELF});
}

void BytecodeGenerationTest::testIfTrueIfFalseNlrArg2() {
    auto bytecodes = methodToBytecode(R"""( test: arg1 with: arg2 = (
            #start.
            self method ifTrue: [ arg1 ] ifFalse: [ ^ arg2 ].
            #end
        ) )""");

    check(bytecodes,
          {BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_SELF, BC(BC_SEND, 1),
           BC(BC_JUMP_ON_FALSE_POP, 7, 0), BC_PUSH_ARG_1, BC(BC_JUMP, 5, 0),
           BC_PUSH_ARG_2, BC_RETURN_LOCAL, BC_POP, BC_PUSH_CONSTANT_2,
           BC_RETURN_SELF});
}
void BytecodeGenerationTest::testInliningOfOr() {
    inliningOfOr("or:");
    inliningOfOr("||");
}

void BytecodeGenerationTest::inliningOfOr(std::string selector) {
    std::string source = "test = ( true OR_SEL [ #val ] )";
    bool wasReplaced = ReplacePattern(source, "OR_SEL", selector);
    assert(wasReplaced);

    auto bytecodes = methodToBytecode(source.data());
    check(bytecodes,
          {BC_PUSH_CONSTANT_0, BC(BC_JUMP_ON_TRUE_POP, 7, 0),
           // true branch
           BC_PUSH_CONSTANT_1,  // push the `#val`
           BC(BC_JUMP, 4, 0),
           // false branch, jump_on_true target, push true
           BC_PUSH_CONSTANT_0,
           // target of the jump in the true branch
           BC_RETURN_SELF});

    tearDown();
}

void BytecodeGenerationTest::testInliningOfAnd() {
    inliningOfAnd("and:");
    inliningOfAnd("&&");
}

void BytecodeGenerationTest::inliningOfAnd(std::string selector) {
    std::string source = "test = ( true AND_SEL [ #val ] )";
    bool wasReplaced = ReplacePattern(source, "AND_SEL", selector);
    assert(wasReplaced);

    auto bytecodes = methodToBytecode(source.data());
    check(bytecodes,
          {BC_PUSH_CONSTANT_0, BC(BC_JUMP_ON_FALSE_POP, 7, 0),
           // true branch
           BC_PUSH_CONSTANT_1,  // push the `#val`
           BC(BC_JUMP, 4, 0),
           // false branch, jump_on_false target, push false
           BC_PUSH_CONSTANT_2,
           // target of the jump in the true branch
           BC_RETURN_SELF});

    tearDown();
}

void BytecodeGenerationTest::testInliningOfToDo() {
    auto bytecodes = methodToBytecode("test = ( 1 to: 2 do: [:i | i ] )");
    check(bytecodes,
          {BC_PUSH_1, BC_PUSH_CONSTANT_0,
           BC_DUP_SECOND,  // stack: Top[1, 2, 1]

           BC(BC_JUMP_IF_GREATER, 11, 0),  // consume only on jump
           BC_DUP,

           BC_POP_LOCAL_0,   // store the i into the local (arg becomes local
                             // after inlining)
           BC_PUSH_LOCAL_0,  // push the local on the stack as part of the
                             // block's code
           BC_POP,           // cleanup after block
           BC_INC,           // increment top, the iteration counter

           // jump back to the jump_if_greater bytecode
           BC(BC_JUMP_BACKWARD, 8, 0),

           // jump_if_greater target
           BC_RETURN_SELF});
}

void BytecodeGenerationTest::testIfArg() {
    ifArg("ifTrue:", BC_JUMP_ON_FALSE_TOP_NIL);
    ifArg("ifFalse:", BC_JUMP_ON_TRUE_TOP_NIL);
}

void BytecodeGenerationTest::ifArg(std::string selector, int8_t jumpBytecode) {
    std::string source = R"""(      test: arg = (
                                         #start.
                                         self method IF_SELECTOR [ arg ].
                                         #end
                                     ) )""";
    bool wasReplaced = ReplacePattern(source, "IF_SELECTOR", selector);
    assert(wasReplaced);

    auto bytecodes = methodToBytecode(source.data());
    check(bytecodes,
          {BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_SELF, BC(BC_SEND, 1),
           BC(jumpBytecode, 4, 0), BC_PUSH_ARG_1, BC_POP, BC_PUSH_CONSTANT_2,
           BC_POP, BC_PUSH_SELF, BC_RETURN_LOCAL});

    tearDown();
}

void BytecodeGenerationTest::testKeywordIfTrueArg() {
    auto bytecodes = methodToBytecode(R"""(      test: arg = (
                                                     #start.
                                                     (self key: 5) ifTrue: [ arg ].
                                                     #end
                                                 ) )""");
    check(bytecodes,
          {BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_SELF, BC_PUSH_CONSTANT_1,
           BC(BC_SEND, 2), BC(BC_JUMP_ON_FALSE_TOP_NIL, 4, 0), BC_PUSH_ARG_1,
           BC_POP, BC(BC_PUSH_CONSTANT, 3), BC_POP, BC_PUSH_SELF,
           BC_RETURN_LOCAL});
}

void BytecodeGenerationTest::testIfReturnNonLocal() {
    ifReturnNonLocal("ifTrue:", BC_JUMP_ON_FALSE_TOP_NIL);
    ifReturnNonLocal("ifFalse:", BC_JUMP_ON_TRUE_TOP_NIL);
}

void BytecodeGenerationTest::ifReturnNonLocal(std::string selector,
                                              int8_t jumpBytecode) {
    std::string source = R"""(      test: arg = (
                                         #start.
                                         self method IF_SELECTOR [ ^ arg ].
                                         #end
                                     ) )""";
    bool wasReplaced = ReplacePattern(source, "IF_SELECTOR", selector);
    assert(wasReplaced);

    auto bytecodes = methodToBytecode(source.data());
    check(bytecodes,
          {BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_SELF, BC(BC_SEND, 1),
           BC(jumpBytecode, 5, 0), BC_PUSH_ARG_1, BC_RETURN_LOCAL, BC_POP,
           BC_PUSH_CONSTANT_2, BC_POP, BC_PUSH_SELF, BC_RETURN_LOCAL});

    tearDown();
}

/*
 @pytest.mark.parametrize(
     "operator,bytecode",
     [
         ("+", Bytecodes.inc),
         ("-", Bytecodes.dec),
     ],
 )
 def test_inc_dec_bytecodes(mgenc, operator, bytecode):
     bytecodes = method_to_bytecodes(mgenc, "test = ( 1 OP 1 )".replace("OP",
 operator))

     assert len(bytecodes) == 3
     check(bytecodes, [Bytecodes.push_1, bytecode, Bytecodes.return_self])


 def test_if_true_and_inc_field(cgenc, mgenc):
     add_field(cgenc, "field")
     bytecodes = method_to_bytecodes(
         mgenc,
         """
         test: arg = (
             #start.
             (self key: 5) ifTrue: [ field := field + 1 ].
             #end
         )""",
     )

     assert len(bytecodes) == 18
     check(
         bytecodes,
         [
             (6, Bytecodes.send_2),
             BC(Bytecodes.jump_on_false_top_nil, 6, note="jump offset"),
             Bytecodes.inc_field_push,
             Bytecodes.pop,
             Bytecodes.push_constant,
         ],
     )


 def test_if_true_and_inc_arg(mgenc):
     bytecodes = method_to_bytecodes(
         mgenc,
         """
         test: arg = (
             #start.
             (self key: 5) ifTrue: [ arg + 1 ].
             #end
         )""",
     )

     assert len(bytecodes) == 19
     check(
         bytecodes,
         [
             (6, Bytecodes.send_2),
             BC(Bytecodes.jump_on_false_top_nil, 7, note="jump offset"),
             BC(Bytecodes.push_argument, 1, 0),
             Bytecodes.inc,
             Bytecodes.pop,
             Bytecodes.push_constant,
         ],
     )


 def test_nested_ifs(cgenc, mgenc):
     add_field(cgenc, "field")
     bytecodes = method_to_bytecodes(
         mgenc,
         """
         test: arg = (
             true ifTrue: [
                 false ifFalse: [
                   ^ field - arg
                 ]
             ]
         )""",
     )

     assert len(bytecodes) == 16
     check(
         bytecodes,
         [
             Bytecodes.push_constant_0,
             BC(Bytecodes.jump_on_false_top_nil, 14, note="jump offset"),
             (5, Bytecodes.jump_on_true_top_nil),
             Bytecodes.push_field_0,
             BC(Bytecodes.push_argument, 1, 0),
             Bytecodes.send_2,
             Bytecodes.return_local,
             Bytecodes.return_self,
         ],
     )


 def test_nested_ifs_and_locals(cgenc, mgenc):
     add_field(cgenc, "field")
     bytecodes = method_to_bytecodes(
         mgenc,
         """
         test: arg = (
           | a b c d |
           a := b.
           true ifTrue: [
             | e f g |
             e := 2.
             c := 3.
             false ifFalse: [
               | h i j |
               h := 1.
               ^ i - j - f - g - d ] ] )""",
     )

     assert len(bytecodes) == 54
     check(
         bytecodes,
         [
             BC(Bytecodes.push_local, 1, 0),
             BC(Bytecodes.pop_local, 0, 0),
             (7, BC(Bytecodes.jump_on_false_top_nil, 46)),
             (12, BC(Bytecodes.pop_local, 4, 0)),
             (17, BC(Bytecodes.pop_local, 2, 0)),
             (22, BC(Bytecodes.jump_on_true_top_nil, 31)),
             (26, BC(Bytecodes.pop_local, 7, 0)),
             (47, BC(Bytecodes.push_local, 3, 0)),
         ],
     )


 def test_nested_ifs_and_non_inlined_blocks(cgenc, mgenc):
     add_field(cgenc, "field")
     bytecodes = method_to_bytecodes(
         mgenc,
         """
         test: arg = (
           | a |
           a := 1.
           true ifTrue: [
             | e |
             e := 0.
             [ a := 1. a ].
             false ifFalse: [
               | h |
               h := 1.
               [ h + a + e ].
               ^ h ] ].

           [ a ]
         )""",
     )

     assert len(bytecodes) == 35
     check(
         bytecodes,
         [
             (4, Bytecodes.push_constant_1),
             BC(Bytecodes.jump_on_false_top_nil, 26),
             (9, BC(Bytecodes.pop_local, 1, 0, note="local e")),
             (17, BC(Bytecodes.jump_on_true_top_nil, 14)),
             (21, BC(Bytecodes.pop_local, 2, 0, note="local h")),
         ],
     )

     check(
         mgenc.get_constant(12).get_bytecodes(),
         [(1, BC(Bytecodes.pop_local, 0, 1, note="local a"))],
     )

     check(
         mgenc.get_constant(24).get_bytecodes(),
         [
             BC(Bytecodes.push_local, 2, 1, note="local h"),
             BC(Bytecodes.push_local, 0, 1, note="local a"),
             (8, BC(Bytecodes.push_local, 1, 1, note="local e")),
         ],
     )

     check(
         mgenc.get_constant(32).get_bytecodes(),
         [BC(Bytecodes.push_local, 0, 1, note="local a")],
     )


 def test_nested_non_inlined_blocks(cgenc, mgenc):
     add_field(cgenc, "field")
     bytecodes = method_to_bytecodes(
         mgenc,
         """
         test: a = ( | b |
           true ifFalse: [ | c |
             a. b. c.
             [:d |
               a. b. c. d.
               [:e |
                 a. b. c. d. e ] ] ]
         )""",
     )

     assert len(bytecodes) == 19
     check(
         bytecodes,
         [
             (1, BC(Bytecodes.jump_on_true_top_nil, 17)),
             BC(Bytecodes.push_argument, 1, 0, note="arg a"),
             (8, BC(Bytecodes.push_local, 0, 0, note="local b")),
             (12, BC(Bytecodes.push_local, 1, 0, note="local c")),
         ],
     )

     block_method = mgenc.get_constant(16)
     check(
         block_method.get_bytecodes(),
         [
             BC(Bytecodes.push_argument, 1, 1, note="arg a"),
             (4, BC(Bytecodes.push_local, 0, 1, note="local b")),
             (8, BC(Bytecodes.push_local, 1, 1, note="local c")),
             (12, BC(Bytecodes.push_argument, 1, 0, note="arg d")),
         ],
     )

     block_method = block_method.get_constant(16)
     check(
         block_method.get_bytecodes(),
         [
             BC(Bytecodes.push_argument, 1, 2, note="arg a"),
             (4, BC(Bytecodes.push_local, 0, 2, note="local b")),
             (8, BC(Bytecodes.push_local, 1, 2, note="local c")),
             (12, BC(Bytecodes.push_argument, 1, 1, note="arg d")),
             (16, BC(Bytecodes.push_argument, 1, 0, note="arg e")),
         ],
     )


 def test_block_if_true_arg(bgenc):
     bytecodes = block_to_bytecodes(
         bgenc,
         """
         [:arg | #start.
             self method ifTrue: [ arg ].
             #end
         ]""",
     )

     assert len(bytecodes) == 17
     check(
         bytecodes,
         [
             (5, Bytecodes.send_1),
             BC(Bytecodes.jump_on_false_top_nil, 6),
             BC(Bytecodes.push_argument, 1, 0),
             Bytecodes.pop,
             Bytecodes.push_constant,
         ],
     )


 def test_block_if_true_method_arg(mgenc, bgenc):
     mgenc.add_argument("arg", None, None)
     bytecodes = block_to_bytecodes(
         bgenc,
         """
         [ #start.
             self method ifTrue: [ arg ].
             #end
         ]""",
     )

     assert len(bytecodes) == 17
     check(
         bytecodes,
         [
             (7, BC(Bytecodes.jump_on_false_top_nil, 6)),
             BC(Bytecodes.push_argument, 1, 1),
             Bytecodes.pop,
             Bytecodes.push_constant,
         ],
     )



 @pytest.mark.parametrize(
     "sel1,sel2,jump_bytecode",
     [
         ("ifTrue:", "ifFalse:", Bytecodes.jump_on_false_pop),
         ("ifFalse:", "ifTrue:", Bytecodes.jump_on_true_pop),
     ],
 )
 def test_if_true_if_false_return(mgenc, sel1, sel2, jump_bytecode):
     bytecodes = method_to_bytecodes(
         mgenc,
         """
         test: arg1 with: arg2 = (
             #start.
             ^ self method SEL1 [ ^ arg1 ] SEL2 [ arg2 ]
         )""".replace(
             "SEL1", sel1
         ).replace(
             "SEL2", sel2
         ),
     )

     assert len(bytecodes) == 21
     check(
         bytecodes,
         [
             (7, BC(jump_bytecode, 10)),
             (14, BC(Bytecodes.jump, 6)),
         ],
     )




 @pytest.mark.parametrize(
     "if_selector,jump_bytecode",
     [
         ("ifTrue:", Bytecodes.jump_on_false_top_nil),
         ("ifFalse:", Bytecodes.jump_on_true_top_nil),
     ],
 )
 def test_block_if_return_non_local(bgenc, if_selector, jump_bytecode):
     bytecodes = block_to_bytecodes(
         bgenc,
         """
         [:arg |
             #start.
             self method IF_SELECTOR [ ^ arg ].
             #end
         ]""".replace(
             "IF_SELECTOR", if_selector
         ),
     )

     assert len(bytecodes) == 19
     check(
         bytecodes,
         [
             (5, Bytecodes.send_1),
             BC(jump_bytecode, 8),
             BC(Bytecodes.push_argument, 1, 0),
             BC(Bytecodes.return_non_local, 1),
             Bytecodes.pop,
         ],
     )




 @pytest.mark.parametrize(
     "source,bytecode",
     [
         ("0", Bytecodes.push_0),
         ("1", Bytecodes.push_1),
         ("-10", BC_PUSH_CONSTANT_2),
         ("3333", BC_PUSH_CONSTANT_2),
         ("'str'", BC_PUSH_CONSTANT_2),
         ("#sym", BC_PUSH_CONSTANT_2),
         ("1.1", BC_PUSH_CONSTANT_2),
         ("-2342.234", BC_PUSH_CONSTANT_2),
         ("true", Bytecodes.push_constant_0),
         ("false", BC_PUSH_CONSTANT_2),
         ("nil", Bytecodes.push_nil),
         ("Nil", Bytecodes.push_global),
         ("UnknownGlobal", Bytecodes.push_global),
         ("[]", Bytecodes.push_block_no_ctx),
     ],
 )
 def test_trivial_method_inlining(mgenc, source, bytecode):
     bytecodes = method_to_bytecodes(mgenc, "test = ( true ifTrue: [ " + source
 + " ] )") check( bytecodes,
         [
             Bytecodes.push_constant_0,
             Bytecodes.jump_on_false_top_nil,
             bytecode,
             Bytecodes.return_self,
         ],
     )


 @pytest.mark.parametrize("field_num", range(0, 7))
 def test_inc_field(cgenc, mgenc, field_num):
     add_field(cgenc, "field0")
     add_field(cgenc, "field1")
     add_field(cgenc, "field2")
     add_field(cgenc, "field3")
     add_field(cgenc, "field4")
     add_field(cgenc, "field5")
     add_field(cgenc, "field6")

     field_name = "field" + str(field_num)
     bytecodes = method_to_bytecodes(
         mgenc, "test = ( " + field_name + " := " + field_name + " + 1 )"
     )

     check(
         bytecodes,
         [
             BC(Bytecodes.inc_field, field_num, 0),
             Bytecodes.return_self,
         ],
     )


 @pytest.mark.parametrize("field_num", range(0, 7))
 def test_inc_field_non_trivial(cgenc, mgenc, field_num):
     add_field(cgenc, "field0")
     add_field(cgenc, "field1")
     add_field(cgenc, "field2")
     add_field(cgenc, "field3")
     add_field(cgenc, "field4")
     add_field(cgenc, "field5")
     add_field(cgenc, "field6")

     field_name = "field" + str(field_num)
     bytecodes = method_to_bytecodes(
         mgenc, "test = ( 1. " + field_name + " := " + field_name + " + 1. 2 )"
     )
     check(
         bytecodes,
         [
             Bytecodes.push_1,
             Bytecodes.pop,
             BC(Bytecodes.inc_field, field_num, 0),
             Bytecodes.push_constant_1,
             Bytecodes.return_self,
         ],
     )


 @pytest.mark.parametrize("field_num", range(0, 7))
 def test_return_inc_field(cgenc, mgenc, field_num):
     add_field(cgenc, "field0")
     add_field(cgenc, "field1")
     add_field(cgenc, "field2")
     add_field(cgenc, "field3")
     add_field(cgenc, "field4")
     add_field(cgenc, "field5")
     add_field(cgenc, "field6")

     field_name = "field" + str(field_num)
     bytecodes = method_to_bytecodes(
         mgenc, "test = ( #foo. ^ " + field_name + " := " + field_name + " + 1
 )"
     )
     check(
         bytecodes,
         [
             Bytecodes.push_constant_0,
             Bytecodes.pop,
             BC(Bytecodes.inc_field_push, field_num, 0),
             Bytecodes.return_local,
         ],
     )


 @pytest.mark.parametrize("field_num", range(0, 7))
 def test_return_inc_field_from_block(cgenc, bgenc, field_num):
     add_field(cgenc, "field0")
     add_field(cgenc, "field1")
     add_field(cgenc, "field2")
     add_field(cgenc, "field3")
     add_field(cgenc, "field4")
     add_field(cgenc, "field5")
     add_field(cgenc, "field6")

     field_name = "field" + str(field_num)
     bytecodes = block_to_bytecodes(
         bgenc, "[ #foo. " + field_name + " := " + field_name + " + 1 ]"
     )

     check(
         bytecodes,
         [
             Bytecodes.push_constant_0,
             Bytecodes.pop,
             BC(Bytecodes.inc_field_push, field_num, 1),
             Bytecodes.return_local,
         ],
     )


 @pytest.mark.parametrize(
     "field_num,bytecode",
     [
         (0, Bytecodes.return_field_0),
         (1, Bytecodes.return_field_1),
         (2, Bytecodes.return_field_2),
         (3, BC(Bytecodes.push_field, 3)),
         (4, BC(Bytecodes.push_field, 4)),
     ],
 )
 def test_return_field(cgenc, mgenc, field_num, bytecode):
     add_field(cgenc, "field0")
     add_field(cgenc, "field1")
     add_field(cgenc, "field2")
     add_field(cgenc, "field3")
     add_field(cgenc, "field4")
     add_field(cgenc, "field5")
     add_field(cgenc, "field6")

     field_name = "field" + str(field_num)
     bytecodes = method_to_bytecodes(mgenc, "test = ( 1. ^ " + field_name + "
 )")

     check(
         bytecodes,
         [
             Bytecodes.push_1,
             Bytecodes.pop,
             bytecode,
         ],
     )





 def test_field_read_inlining(cgenc, mgenc):
     add_field(cgenc, "field")
     bytecodes = method_to_bytecodes(mgenc, "test = ( true and: [ field ] )")

     assert len(bytecodes) == 10
     check(
         bytecodes,
         [
             Bytecodes.push_constant_0,
             BC(Bytecodes.jump_on_false_pop, 7),
             # true branch
             Bytecodes.push_field_0,
             BC(Bytecodes.jump, 4),
             # false branch, jump_on_true target, push true
             BC_PUSH_CONSTANT_2,
             # target of the jump in the true branch
             Bytecodes.return_self,
         ],
     )


 */

void BytecodeGenerationTest::testJumpQueuesOrdering() {
    std::priority_queue<Jump> jumps;

    jumps.emplace(Jump(1, BC_JUMP, 0));
    jumps.emplace(Jump(5, BC_JUMP, 0));
    jumps.emplace(Jump(8, BC_JUMP, 0));
    jumps.emplace(Jump(2, BC_JUMP, 0));

    CPPUNIT_ASSERT_EQUAL((size_t)1, jumps.top().originalJumpTargetIdx);
    jumps.pop();
    CPPUNIT_ASSERT_EQUAL((size_t)2, jumps.top().originalJumpTargetIdx);
    jumps.pop();
    CPPUNIT_ASSERT_EQUAL((size_t)5, jumps.top().originalJumpTargetIdx);
    jumps.pop();
    CPPUNIT_ASSERT_EQUAL((size_t)8, jumps.top().originalJumpTargetIdx);
    jumps.pop();

    std::priority_queue<BackJump> backJumps;
    backJumps.emplace(BackJump(13, 9));
    backJumps.emplace(BackJump(3, 12));
    backJumps.emplace(BackJump(54, 54));

    CPPUNIT_ASSERT_EQUAL((size_t)3, backJumps.top().loopBeginIdx);
    backJumps.pop();
    CPPUNIT_ASSERT_EQUAL((size_t)13, backJumps.top().loopBeginIdx);
    backJumps.pop();
    CPPUNIT_ASSERT_EQUAL((size_t)54, backJumps.top().loopBeginIdx);
    backJumps.pop();

    std::priority_queue<BackJumpPatch> backJumpsToPatch;
    backJumpsToPatch.emplace(BackJumpPatch(3, 2));
    backJumpsToPatch.emplace(BackJumpPatch(32, 44));
    backJumpsToPatch.emplace(BackJumpPatch(12, 55));

    CPPUNIT_ASSERT_EQUAL((size_t)3, backJumpsToPatch.top().backwardsJumpIdx);
    backJumpsToPatch.pop();
    CPPUNIT_ASSERT_EQUAL((size_t)12, backJumpsToPatch.top().backwardsJumpIdx);
    backJumpsToPatch.pop();
    CPPUNIT_ASSERT_EQUAL((size_t)32, backJumpsToPatch.top().backwardsJumpIdx);
    backJumpsToPatch.pop();
}
