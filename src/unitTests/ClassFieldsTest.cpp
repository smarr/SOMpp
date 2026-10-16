#include "ClassFieldsTest.h"

#include <cppunit/TestAssert.h>
#include <cstddef>

#include "../compiler/SourcecodeCompiler.h"
#include "../vm/Globals.h"
#include "../vm/Symbols.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMArray.h"
#include "../vmobjects/VMClass.h"

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
