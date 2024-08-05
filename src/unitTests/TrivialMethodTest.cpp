#include "TrivialMethodTest.h"

#include <cppunit/TestAssert.h>
#include <string>

#include "../compiler/MethodGenerationContext.h"
#include "../vm/IsValidObject.h"
#include "../vmobjects/VMInvokable.h"

void TrivialMethodTest::literalReturn(std::string source) {
    std::string s = "test = ( ^ " + source + " )";
    methodToBytecode(s.data());
    VMInvokable* m = _mgenc->Assemble();

    std::string expected = "Expected to be trivial: " + s;
    bool result = IsLiteralReturn(m);
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

void TrivialMethodTest::blockLiteralReturn(std::string source) {
    std::string s = "[ " + source + " ]";
    blockToBytecode(s.data());
    VMInvokable* m = _bgenc->Assemble();

    std::string expected = "Expected to be trivial: " + s;
    bool result = IsLiteralReturn(m);
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

void TrivialMethodTest::literalNoReturn(std::string source) {
    std::string s = "test = ( " + source + " )";
    methodToBytecode(s.data());
    VMInvokable* m = _mgenc->Assemble();

    std::string expected = "Expected to be non-trivial: " + s;
    bool result = IsLiteralReturn(m);
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

void TrivialMethodTest::nonTrivialLiteralReturn(std::string source) {
    std::string s = "test = ( 1. ^ " + source + " )";
    methodToBytecode(s.data());
    VMInvokable* m = _mgenc->Assemble();

    std::string expected = "Expected to be non-trivial: " + s;
    bool result = !IsLiteralReturn(m);
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

void TrivialMethodTest::globalReturn(std::string source) {
    std::string s = "test = ( ^ " + source + " )";
    methodToBytecode(s.data());
    VMInvokable* m = _mgenc->Assemble();

    std::string expected = "Expected to be trivial: " + s;
    bool result = IsGlobalReturn(m);
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
    bool result = !IsGlobalReturn(m);
    CPPUNIT_ASSERT_MESSAGE(expected.data(), result);
}

void TrivialMethodTest::testUnknownGlobalInBlock() {
    blockToBytecode("[ UnknownGlobalSSSS ]");
    VMInvokable* m = _bgenc->Assemble();

    std::string expected = "Expected to be trivial: [ UnknownGlobalSSSS ]";
    bool result = IsGlobalReturn(m);
    CPPUNIT_ASSERT_MESSAGE(expected.data(), result);
}

/*
 def test_field_getter_0(cgenc, mgenc):
     add_field(cgenc, "field")
     body_or_none = parse_method(mgenc, "test = ( ^ field )")
     m = mgenc.assemble(body_or_none)
     assert isinstance(m, FieldRead)


 def test_field_getter_n(cgenc, mgenc):
     add_field(cgenc, "a")
     add_field(cgenc, "b")
     add_field(cgenc, "c")
     add_field(cgenc, "d")
     add_field(cgenc, "e")
     add_field(cgenc, "field")
     body_or_none = parse_method(mgenc, "test = ( ^ field )")
     m = mgenc.assemble(body_or_none)
     assert isinstance(m, FieldRead)


 def test_non_trivial_getter_0(cgenc, mgenc):
     add_field(cgenc, "field")
     body = parse_method(mgenc, "test = ( 0. ^ field )")
     m = mgenc.assemble(body)
     assert isinstance(m, AstMethod) or isinstance(m, BcMethod)


 def test_non_trivial_getter_n(cgenc, mgenc):
     add_field(cgenc, "a")
     add_field(cgenc, "b")
     add_field(cgenc, "c")
     add_field(cgenc, "d")
     add_field(cgenc, "e")
     add_field(cgenc, "field")
     body = parse_method(mgenc, "test = ( 0. ^ field )")
     m = mgenc.assemble(body)
     assert isinstance(m, AstMethod) or isinstance(m, BcMethod)


 @pytest.mark.parametrize(
     "source", ["field := val", "field := val.", "field := val. ^ self"]
 )
 def test_field_setter_0(cgenc, mgenc, source):
     add_field(cgenc, "field")
     body_or_none = parse_method(mgenc, "test: val = ( " + source + " )")
     m = mgenc.assemble(body_or_none)
     assert isinstance(m, FieldWrite)


 @pytest.mark.parametrize(
     "source", ["field := value", "field := value.", "field := value. ^ self"]
 )
 def test_field_setter_n(cgenc, mgenc, source):
     add_field(cgenc, "a")
     add_field(cgenc, "b")
     add_field(cgenc, "c")
     add_field(cgenc, "d")
     add_field(cgenc, "e")
     add_field(cgenc, "field")
     body_or_none = parse_method(mgenc, "test: value = ( " + source + " )")
     m = mgenc.assemble(body_or_none)
     assert isinstance(m, FieldWrite)


 def test_non_trivial_field_setter_0(cgenc, mgenc):
     add_field(cgenc, "field")
     body_or_none = parse_method(mgenc, "test: val = ( 0. field := value )")
     m = mgenc.assemble(body_or_none)
     assert isinstance(m, AstMethod) or isinstance(m, BcMethod)


 def test_non_trivial_field_setter_n(cgenc, mgenc):
     add_field(cgenc, "a")
     add_field(cgenc, "b")
     add_field(cgenc, "c")
     add_field(cgenc, "d")
     add_field(cgenc, "e")
     add_field(cgenc, "field")
     body_or_none = parse_method(mgenc, "test: val = ( 0. field := value )")
     m = mgenc.assemble(body_or_none)
     assert isinstance(m, AstMethod) or isinstance(m, BcMethod)

 def test_block_return(mgenc):
     body_or_none = parse_method(mgenc, "test = ( ^ [] )")
     m = mgenc.assemble(body_or_none)
     assert isinstance(m, AstMethod) or isinstance(m, BcMethod)


 */
