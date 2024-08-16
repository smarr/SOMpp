/*
 *
 *
 Copyright (c) 2007 Michael Haupt, Tobias Pape, Arne Bergmann
 Software Architecture Group, Hasso Plattner Institute, Potsdam, Germany
 http://www.hpi.uni-potsdam.de/swa/

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

#include "ClassGenerationContext.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>

#include "../misc/VectorUtil.h"
#include "../vm/Globals.h"
#include "../vm/IsValidObject.h"
#include "../vm/Symbols.h"
#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMArray.h"
#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMInvokable.h"
#include "../vmobjects/VMSymbol.h"

ClassGenerationContext::ClassGenerationContext()
    : name(nullptr), superName(nullptr), classSide(false), instanceFields(),
      instanceMethods(), classFields(), classMethods() {}

void ClassGenerationContext::AddClassField(VMSymbol* field) {
    classFields.push_back(field);
}

void ClassGenerationContext::AddInstanceField(VMSymbol* field) {
    instanceFields.push_back(field);
}

void ClassGenerationContext::SetInstanceFieldsOfSuper(VMArray* fields) {
    size_t const num = fields->GetNumberOfIndexableFields();
    for (size_t i = 0; i < num; i++) {
        auto* fieldName = (VMSymbol*)fields->GetIndexableField(i);
        assert(IsVMSymbol(fieldName));
        instanceFields.push_back(fieldName);
    }
}

void ClassGenerationContext::SetClassFieldsOfSuper(VMArray* fields) {
    size_t const num = fields->GetNumberOfIndexableFields();
    for (size_t i = 0; i < num; i++) {
        auto* fieldName = (VMSymbol*)fields->GetIndexableField(i);
        assert(IsVMSymbol(fieldName));
        classFields.push_back(fieldName);
    }
}

bool ClassGenerationContext::HasField(VMSymbol* field) {
    if (IsClassSide()) {
        return Contains(classFields, field);
    }
    return Contains(instanceFields, field);
}

int64_t ClassGenerationContext::GetFieldIndex(VMSymbol* field) {
    if (IsClassSide()) {
        return IndexOf(classFields, field);
    }
    return IndexOf(instanceFields, field);
}

void ClassGenerationContext::AddInstanceMethod(VMInvokable* method) {
    instanceMethods.push_back(method);
}

void ClassGenerationContext::AddClassMethod(VMInvokable* method) {
    classMethods.push_back(method);
}

VMClass* ClassGenerationContext::Assemble() {
    // build class class name
    std::string const ccname = string(name->GetStdString()) + " class";

    // Load the super class
    VMClass* superClass = Universe::LoadClass(superName);

    // Allocate the class of the resulting class
    VMClass* resultClass = Universe::NewClass(load_ptr(metaClassClass));

    // Initialize the class of the resulting class
    resultClass->SetInstanceFields(Universe::NewArrayList(classFields));
    resultClass->SetInstanceInvokables(Universe::NewArrayList(classMethods));
    resultClass->SetName(SymbolFor(ccname));

    VMClass* superMClass = superClass->GetClass();
    resultClass->SetSuperClass(superMClass);

    // Allocate the resulting class
    VMClass* result = Universe::NewClass(resultClass);

    // Initialize the resulting class
    result->SetInstanceFields(Universe::NewArrayList(instanceFields));
    result->SetInstanceInvokables(Universe::NewArrayList(instanceMethods));
    result->SetName(name);
    result->SetSuperClass(superClass);

    return result;
}

void ClassGenerationContext::AssembleSystemClass(VMClass* systemClass) {
    systemClass->SetInstanceInvokables(Universe::NewArrayList(instanceMethods));
    systemClass->SetInstanceFields(Universe::NewArrayList(instanceFields));
    // class-bound == class-instance-bound
    VMClass* superMClass = systemClass->GetClass();
    superMClass->SetInstanceInvokables(Universe::NewArrayList(classMethods));
    superMClass->SetInstanceFields(Universe::NewArrayList(classFields));
}
