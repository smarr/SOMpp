#include "TestWithParsing.h"

#include <cassert>
#include <cppunit/TestAssert.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

#include "../compiler/ClassGenerationContext.h"
#include "../compiler/Disassembler.h"
#include "../compiler/MethodGenerationContext.h"
#include "../compiler/Parser.h"
#include "../interpreter/bytecodes.h"
#include "../vm/Symbols.h"
#include "../vmobjects/VMMethod.h"

void TestWithParsing::dump(MethodGenerationContext* mgenc) {
    if (mgenc == nullptr) {
        if (_bgenc != nullptr) {
            mgenc = _bgenc;
        } else {
            mgenc = _mgenc;
        }
    }
    Disassembler::DumpMethod(mgenc, "");
}

void TestWithParsing::ensureCGenC() {
    if (_cgenc != nullptr) {
        return;
    }

    _cgenc = new ClassGenerationContext();
    _cgenc->SetName(SymbolFor("Test"));
}

void TestWithParsing::ensureMGenC() {
    if (_mgenc != nullptr) {
        return;
    }
    ensureCGenC();

    _mgenc = new MethodGenerationContext(*_cgenc);
    std::string self = strSelf;
    _mgenc->AddArgument(self, {0, 0});
}

void TestWithParsing::ensureBGenC() {
    if (_bgenc != nullptr) {
        return;
    }
    ensureCGenC();
    ensureMGenC();

    _mgenc->SetSignature(SymbolFor("test"));
    _bgenc = new MethodGenerationContext(*_cgenc, _mgenc);
}

void TestWithParsing::addField(const char* fieldName) {
    ensureCGenC();
    _cgenc->AddInstanceField(SymbolFor(fieldName));
}

std::vector<uint8_t> TestWithParsing::methodToBytecode(const char* source,
                                                       bool dumpBytecodes) {
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

std::vector<uint8_t> TestWithParsing::blockToBytecode(const char* source,
                                                      bool dumpBytecodes) {
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

void TestWithParsing::check(std::vector<uint8_t> actual,
                            std::vector<BC>
                                expected) {
    size_t i = 0;
    size_t bci = 0;
    for (; bci < actual.size() && i < expected.size();) {
        uint8_t const actualBc = actual.at(bci);
        uint8_t const bcLength = Bytecode::GetBytecodeLength(actualBc);

        BC const expectedBc = expected.at(i);

        char msg[1000];
        (void)snprintf(msg, 1000,
                       "Bytecode no %zu (at: %zu) expected %s but got %s", i,
                       bci, Bytecode::GetBytecodeName(expectedBc.bytecode),
                       Bytecode::GetBytecodeName(actualBc));
        if (expectedBc.bytecode != actualBc) {
            dump();
        }
        CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, expectedBc.bytecode, actualBc);

        (void)snprintf(
            msg, 1000,
            "Bytecode %zu (%s) was expected to have length %zu, but had %zu", i,
            Bytecode::GetBytecodeName(actualBc), expectedBc.size,
            (size_t)bcLength);

        if (expectedBc.size != bcLength) {
            dump();
        }
        CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, expectedBc.size, (size_t)bcLength);

        if (bcLength > 1) {
            (void)snprintf(msg, 1000,
                           "Bytecode %zu (%s), arg1 expected %hhu but got %hhu",
                           i, Bytecode::GetBytecodeName(expectedBc.bytecode),
                           expectedBc.arg1, actual.at(bci + 1));
            if (expectedBc.arg1 != actual.at(bci + 1)) {
                dump();
            }
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, expectedBc.arg1,
                                         actual.at(bci + 1));

            if (bcLength > 2) {
                (void)snprintf(
                    msg, 1000,
                    "Bytecode %zu (%s), arg2 expected %hhu but got %hhu", i,
                    Bytecode::GetBytecodeName(expectedBc.bytecode),
                    expectedBc.arg2, actual.at(bci + 2));
                if (expectedBc.arg2 != actual.at(bci + 2)) {
                    dump();
                }
                CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, expectedBc.arg2,
                                             actual.at(bci + 2));
            }
        }

        i += 1;
        bci += bcLength;
    }
    if (expected.size() != i || actual.size() != bci) {
        dump();
    }

    CPPUNIT_ASSERT_EQUAL_MESSAGE("All expected bytecodes covered",
                                 expected.size(), i);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("All actual bytecodes covered", actual.size(),
                                 bci);
}
