#include "TrivialMethodTest.h"

#include <cppunit/TestAssert.h>
#include <cstddef>
#include <string>

#include "../compiler/MethodGenerationContext.h"
#include "../vm/IsValidObject.h"
#include "../vmobjects/VMInvokable.h"
#include "../vmobjects/VMTrivialMethod.h"

void TrivialMethodTest::literalReturn(const std::string& source) {
    std::string s = "test = ( ^ " + source + " )";
    methodToBytecode(s.data());
    VMInvokable* m = _mgenc->Assemble();

    std::string expected = "Expected to be trivial: " + s;
    bool const result = IsLiteralReturn(m);
    CPPUNIT_ASSERT_MESSAGE(expected.data(), result);

    tearDown();
}

void TrivialMethodTest::testLiteralReturn() {
    literalReturn("0");
    literalReturn("1");
    literalReturn("-10");
    literalReturn("3333");
    literalReturn("'str'");
    literalReturn("#sym");
    literalReturn("1.1");
    literalReturn("-2342.234");
    literalReturn("true");
    literalReturn("false");
    literalReturn("nil");
}

void TrivialMethodTest::blockLiteralReturn(const std::string& source) {
    std::string s = "[ " + source + " ]";
    blockToBytecode(s.data());
    VMInvokable* m = _bgenc->Assemble();

    std::string expected = "Expected to be trivial: " + s;
    bool const result = IsLiteralReturn(m);
    CPPUNIT_ASSERT_MESSAGE(expected.data(), result);

    tearDown();
}

void TrivialMethodTest::testBlockLiteralReturn() {
    blockLiteralReturn("0");
    blockLiteralReturn("1");
    blockLiteralReturn("-10");
    blockLiteralReturn("3333");
    blockLiteralReturn("'str'");
    blockLiteralReturn("#sym");
    blockLiteralReturn("1.1");
    blockLiteralReturn("-2342.234");
    blockLiteralReturn("true");
    blockLiteralReturn("false");
    blockLiteralReturn("nil");
}

void TrivialMethodTest::literalNoReturn(const std::string& source) {
    std::string s = "test = ( " + source + " )";
    methodToBytecode(s.data());
    VMInvokable* m = _mgenc->Assemble();

    std::string expected = "Expected to be non-trivial: " + s;
    bool const result = IsLiteralReturn(m);
    CPPUNIT_ASSERT_MESSAGE(expected.data(), !result);

    tearDown();
}

void TrivialMethodTest::testLiteralNoReturn() {
    literalNoReturn("0");
    literalNoReturn("1");
    literalNoReturn("-10");
    literalNoReturn("3333");
    literalNoReturn("'str'");
    literalNoReturn("#sym");
    literalNoReturn("1.1");
    literalNoReturn("-2342.234");
    literalNoReturn("true");
    literalNoReturn("false");
    literalNoReturn("nil");
}

void TrivialMethodTest::nonTrivialLiteralReturn(const std::string& source) {
    std::string s = "test = ( 1. ^ " + source + " )";
    methodToBytecode(s.data());
    VMInvokable* m = _mgenc->Assemble();

    std::string expected = "Expected to be non-trivial: " + s;
    bool const result = !IsLiteralReturn(m);
    CPPUNIT_ASSERT_MESSAGE(expected.data(), result);

    tearDown();
}

void TrivialMethodTest::testNonTrivialLiteralReturn() {
    nonTrivialLiteralReturn("0");
    nonTrivialLiteralReturn("1");
    nonTrivialLiteralReturn("-10");
    nonTrivialLiteralReturn("3333");
    nonTrivialLiteralReturn("'str'");
    nonTrivialLiteralReturn("#sym");
    nonTrivialLiteralReturn("1.1");
    nonTrivialLiteralReturn("-2342.234");
    nonTrivialLiteralReturn("true");
    nonTrivialLiteralReturn("false");
    nonTrivialLiteralReturn("nil");
}

void TrivialMethodTest::globalReturn(const std::string& source) {
    std::string s = "test = ( ^ " + source + " )";
    methodToBytecode(s.data());
    VMInvokable* m = _mgenc->Assemble();

    std::string expected = "Expected to be trivial: " + s;
    bool const result = IsGlobalReturn(m);
    CPPUNIT_ASSERT_MESSAGE(expected.data(), result);

    tearDown();
}

void TrivialMethodTest::testGlobalReturn() {
    globalReturn("Nil");
    globalReturn("system");
    globalReturn("MyClassFooBar");
}

void TrivialMethodTest::testNonTrivialGlobalReturn() {
    methodToBytecode("test = ( #foo. ^ system )");
    VMInvokable* m = _mgenc->Assemble();

    std::string expected =
        "Expected to be non-trivial: test = ( #foo. ^ system )";
    bool const result = !IsGlobalReturn(m);
    CPPUNIT_ASSERT_MESSAGE(expected.data(), result);
}

void TrivialMethodTest::testUnknownGlobalInBlock() {
    blockToBytecode("[ UnknownGlobalSSSS ]");
    VMInvokable* m = _bgenc->Assemble();

    std::string expected = "Expected to be trivial: [ UnknownGlobalSSSS ]";
    bool const result = IsGlobalReturn(m);
    CPPUNIT_ASSERT_MESSAGE(expected.data(), result);
}

void TrivialMethodTest::testFieldGetter0() {
    addField("field");
    methodToBytecode("test = ( ^ field )");

    VMInvokable* m = _mgenc->Assemble();

    std::string expected = "Expected to be trivial: test = ( ^ field )";
    bool const result = IsGetter(m);
    CPPUNIT_ASSERT_MESSAGE(expected.data(), result);
}

void TrivialMethodTest::testFieldGetterN() {
    addField("a");
    addField("b");
    addField("c");
    addField("d");
    addField("e");
    addField("field");
    methodToBytecode("test = ( ^ field )");

    VMInvokable* m = _mgenc->Assemble();

    std::string expected = "Expected to be trivial: test = ( ^ field )";
    bool const result = IsGetter(m);
    CPPUNIT_ASSERT_MESSAGE(expected.data(), result);
}

void TrivialMethodTest::testNonTrivialFieldGetter0() {
    addField("field");
    methodToBytecode("test = ( 0. ^ field )");

    VMInvokable* m = _mgenc->Assemble();

    std::string expected = "Expected to be non-trivial: test = ( 0. ^ field )";
    bool const result = !IsGetter(m);
    CPPUNIT_ASSERT_MESSAGE(expected.data(), result);
}

void TrivialMethodTest::testNonTrivialFieldGetterN() {
    addField("a");
    addField("b");
    addField("c");
    addField("d");
    addField("e");
    addField("field");
    methodToBytecode("test = ( 0. ^ field )");

    VMInvokable* m = _mgenc->Assemble();

    std::string expected = "Expected to be non-trivial: test = ( 0. ^ field )";
    bool const result = !IsGetter(m);
    CPPUNIT_ASSERT_MESSAGE(expected.data(), result);
}

void TrivialMethodTest::testFieldSetter0() {
    fieldSetter0("field := val");
    fieldSetter0("field := val.");
    fieldSetter0("field := val. ^ self");
}

void TrivialMethodTest::fieldSetter0(const std::string& source) {
    addField("field");
    std::string s = "test: val = ( " + source + " )";
    methodToBytecode(s.data());

    VMInvokable* m = _mgenc->Assemble();

    std::string expected = "Expected to be trivial: " + s;
    bool const result = IsSetter(m);
    CPPUNIT_ASSERT_MESSAGE(expected.data(), result);

    auto* setter = (VMSetter*)m;
    CPPUNIT_ASSERT_EQUAL(setter->fieldIndex, (size_t)0);
    CPPUNIT_ASSERT_EQUAL(setter->argIndex, (size_t)1);
    CPPUNIT_ASSERT_EQUAL((size_t)setter->numberOfArguments, (size_t)2);

    tearDown();
}

void TrivialMethodTest::testFieldSetterN() {
    fieldSetterN("field := arg2");
    fieldSetterN("field := arg2.");
    fieldSetterN("field := arg2. ^ self");
}

void TrivialMethodTest::fieldSetterN(const std::string& source) {
    addField("a");
    addField("b");
    addField("c");
    addField("d");
    addField("e");
    addField("field");
    std::string s = "a: arg1 b: arg2 c: arg3 = ( " + source + " )";
    methodToBytecode(s.data());

    VMInvokable* m = _mgenc->Assemble();

    std::string expected = "Expected to be trivial: " + s;
    bool const result = IsSetter(m);
    CPPUNIT_ASSERT_MESSAGE(expected.data(), result);

    auto* setter = (VMSetter*)m;
    CPPUNIT_ASSERT_EQUAL(setter->fieldIndex, (size_t)5);
    CPPUNIT_ASSERT_EQUAL(setter->argIndex, (size_t)2);
    CPPUNIT_ASSERT_EQUAL((size_t)setter->numberOfArguments, (size_t)4);

    tearDown();
}

void TrivialMethodTest::testNonTrivialFieldSetter0() {
    addField("field");
    std::string s = "test: val = ( 0. field := val )";
    methodToBytecode(s.data());

    VMInvokable* m = _mgenc->Assemble();

    std::string expected = "Expected to be non-trivial: " + s;
    bool const result = !IsSetter(m);
    CPPUNIT_ASSERT_MESSAGE(expected.data(), result);
}

void TrivialMethodTest::testNonTrivialFieldSetterN() {
    addField("a");
    addField("b");
    addField("c");
    addField("d");
    addField("e");
    addField("field");
    std::string s = "test: val = ( 0. field := val )";
    methodToBytecode(s.data());

    VMInvokable* m = _mgenc->Assemble();

    std::string expected = "Expected to be non-trivial: " + s;
    bool const result = !IsSetter(m);
    CPPUNIT_ASSERT_MESSAGE(expected.data(), result);
}

void TrivialMethodTest::testBlockReturn() {
    methodToBytecode("test = ( ^ [] )");

    VMInvokable* m = _mgenc->Assemble();

    std::string expected = "Expected to be non-trivial: test = ( ^ [] )";
    bool const result = IsVMMethod(m);
    CPPUNIT_ASSERT_MESSAGE(expected.data(), result);
}
