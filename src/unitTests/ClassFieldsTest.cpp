#include "ClassFieldsTest.h"

#include <cppunit/TestAssert.h>
#include <cstddef>
#include <cstdint>

#include "../compiler/SourcecodeCompiler.h"
#include "../vm/Globals.h"
#include "../vm/Symbols.h"
#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMArray.h"
#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMObject.h"

void ClassFieldsTest::testClassWithoutClassFields() {
    VMArray const* fields =
        load_ptr(objectClass)->GetClass()->GetInstanceFields();
    CPPUNIT_ASSERT_EQUAL(VMClass::VMClassNumberOfFields,
                         fields->GetNumberOfIndexableFields());
}

void ClassFieldsTest::testClassFieldsFollowFieldsOfVMClass() {
    VMClass const* cls = SourcecodeCompiler::CompileClassString(
        "ClassFieldsTestClass = ( ---- | a b | )", nullptr);
    VMArray const* fields = cls->GetClass()->GetInstanceFields();

    size_t const numVMClassFields = VMClass::VMClassNumberOfFields;
    CPPUNIT_ASSERT_EQUAL(numVMClassFields + 2,
                         fields->GetNumberOfIndexableFields());
    CPPUNIT_ASSERT_EQUAL((vm_oop_t)SymbolFor("a"),
                         fields->GetIndexableField(numVMClassFields));
    CPPUNIT_ASSERT_EQUAL((vm_oop_t)SymbolFor("b"),
                         fields->GetIndexableField(numVMClassFields + 1));
}

VMClass* ClassFieldsTest::compileSubclass() {
    VMClass* super = SourcecodeCompiler::CompileClassString(
        "ClassFieldsTestSuper = ( | a | ---- | x | )", nullptr);
    Universe::SetGlobal(SymbolFor("ClassFieldsTestSuper"), super);
    return SourcecodeCompiler::CompileClassString(
        "ClassFieldsTestSub = ClassFieldsTestSuper ( | b c | ---- | y z | )",
        nullptr);
}

void ClassFieldsTest::testSuperclassFieldsAreCountedOnce() {
    VMClass* cls = compileSubclass();

    CPPUNIT_ASSERT_EQUAL((size_t)3, cls->GetNumberOfInstanceFields());

    VMObject const* obj = Universe::NewInstance(cls);
    CPPUNIT_ASSERT_EQUAL((size_t)3, obj->GetNumberOfFields());
}

void ClassFieldsTest::testFieldNamesOfSubclass() {
    VMClass const* cls = compileSubclass();

    CPPUNIT_ASSERT_EQUAL(SymbolFor("a"), cls->GetInstanceFieldName(0));
    CPPUNIT_ASSERT_EQUAL(SymbolFor("b"), cls->GetInstanceFieldName(1));
    CPPUNIT_ASSERT_EQUAL(SymbolFor("c"), cls->GetInstanceFieldName(2));

    CPPUNIT_ASSERT_EQUAL((int64_t)0, cls->LookupFieldIndex(SymbolFor("a")));
    CPPUNIT_ASSERT_EQUAL((int64_t)1, cls->LookupFieldIndex(SymbolFor("b")));
    CPPUNIT_ASSERT_EQUAL((int64_t)2, cls->LookupFieldIndex(SymbolFor("c")));
}

void ClassFieldsTest::testClassSideFieldsAreCountedOnce() {
    VMClass const* cls = compileSubclass();

    size_t const numVMClassFields = VMClass::VMClassNumberOfFields;
    CPPUNIT_ASSERT_EQUAL(numVMClassFields + 3,
                         cls->GetClass()->GetNumberOfInstanceFields());
    CPPUNIT_ASSERT_EQUAL(numVMClassFields + 3, cls->GetNumberOfFields());
}

void ClassFieldsTest::testClassSideFieldNamesOfSubclass() {
    VMClass const* cls = compileSubclass();
    VMClass const* metaclass = cls->GetClass();

    size_t const numVMClassFields = VMClass::VMClassNumberOfFields;
    CPPUNIT_ASSERT_EQUAL(SymbolFor("x"),
                         metaclass->GetInstanceFieldName(numVMClassFields));
    CPPUNIT_ASSERT_EQUAL(SymbolFor("y"),
                         metaclass->GetInstanceFieldName(numVMClassFields + 1));
    CPPUNIT_ASSERT_EQUAL(SymbolFor("z"),
                         metaclass->GetInstanceFieldName(numVMClassFields + 2));

    auto const index = (int64_t)numVMClassFields;
    CPPUNIT_ASSERT_EQUAL(index, metaclass->LookupFieldIndex(SymbolFor("x")));
    CPPUNIT_ASSERT_EQUAL(index + 1,
                         metaclass->LookupFieldIndex(SymbolFor("y")));
    CPPUNIT_ASSERT_EQUAL(index + 2,
                         metaclass->LookupFieldIndex(SymbolFor("z")));
}
