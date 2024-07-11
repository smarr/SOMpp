#include "BytecodeGenerationTest.h"

#include <cppunit/extensions/HelperMacros.h>

#include <compiler/Disassembler.h>
#include <compiler/Parser.h>
#include <vm/Universe.h>

void BytecodeGenerationTest::dump(MethodGenerationContext* mgenc) {
    Disassembler::DumpMethod(mgenc, "");
}

ClassGenerationContext* BytecodeGenerationTest::makeCGenC() {
    _cgenc = new ClassGenerationContext();
    _cgenc->SetName(GetUniverse()->SymbolFor("Test"));
    return _cgenc;
}

MethodGenerationContext* BytecodeGenerationTest::makeMGenC() {
    _mgenc = new MethodGenerationContext();
    _mgenc->SetHolder(makeCGenC());
    _mgenc->AddArgument("self");
    return _mgenc;
}

std::vector<uint8_t> BytecodeGenerationTest::methodToBytecode(MethodGenerationContext* mgenc, const char* source, bool dumpBytecodes) {
    istringstream ss(source);
    
    StdString fileName = "test";
    Parser parser(ss, fileName);
    parser.method(mgenc);
    
    if (dumpBytecodes) {
        dump(mgenc);
    }
    return mgenc->GetBytecodes();
}



void BytecodeGenerationTest::testEmptyMethodReturnsSelf() {
    auto mgenc = makeMGenC();
    
    auto bytecodes = methodToBytecode(mgenc, "test = ( )");
    
    CPPUNIT_ASSERT_EQUAL((size_t) 4, bytecodes.size());
    
    check(bytecodes, {
        BC_PUSH_ARGUMENT, 0, 0,
        BC_RETURN_LOCAL});
}

void BytecodeGenerationTest::check(std::vector<uint8_t> actual, std::vector<uint8_t> expected) {
    
    for (size_t i = 0; i < actual.size() && i < expected.size(); i += 1) {
        uint8_t actualBc = actual.at(i);
        uint8_t bcLength = Bytecode::GetBytecodeLength(actualBc);
        
        uint8_t expectedBc = expected.at(i);
        
        char msg[1000];
        snprintf(msg, 1000, "Bytecode %zu expected %s but got %s",
                 i,
                 Bytecode::GetBytecodeName(expectedBc),
                 Bytecode::GetBytecodeName(actualBc));
        CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, expectedBc, actualBc);
        
        if (bcLength > 1) {
            snprintf(msg, 1000, "Bytecode %zu (%s), arg1 expected %hhu but got %hhu",
                     i,
                     Bytecode::GetBytecodeName(expectedBc),
                     expected.at(i + 1),
                     actual.at(i + 1));
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, expectedBc, actualBc);
            
            i += 1;
            
            if (bcLength > 2) {
                snprintf(msg, 1000, "Bytecode %zu (%s), arg2 expected %hhu but got %hhu",
                         i,
                         Bytecode::GetBytecodeName(expectedBc),
                         expected.at(i + 1),
                         actual.at(i + 1));
                CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, expectedBc, actualBc);
                
                i += 1;
            }
        }
    }

    CPPUNIT_ASSERT_EQUAL(expected.size(), actual.size());
}

