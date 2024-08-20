#pragma once

#include <cppunit/extensions/HelperMacros.h>

#include "../compiler/ClassGenerationContext.h"
#include "../compiler/MethodGenerationContext.h"

class BC {
public:
    // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
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

class TestWithParsing : public CPPUNIT_NS::TestCase {
public:
    inline void setUp() override {}

    inline void tearDown() override {
        delete _cgenc;
        _cgenc = nullptr;

        delete _mgenc;
        _mgenc = nullptr;

        delete _bgenc;
        _bgenc = nullptr;
    }

protected:
    ClassGenerationContext* _cgenc = nullptr;
    MethodGenerationContext* _mgenc = nullptr;
    MethodGenerationContext* _bgenc = nullptr;

    void ensureCGenC();
    void ensureMGenC();
    void ensureBGenC();
    void addField(const char* fieldName);

    std::vector<uint8_t> methodToBytecode(const char* source,
                                          bool dumpBytecodes = false);
    std::vector<uint8_t> blockToBytecode(const char* source,
                                         bool dumpBytecodes = false);

    void dump(MethodGenerationContext* mgenc = nullptr);

    void check(std::vector<uint8_t> actual, std::vector<BC> expected);
};
