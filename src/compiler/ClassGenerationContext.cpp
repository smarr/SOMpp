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

#include <misc/VectorUtil.h>

#include "ClassGenerationContext.h"

#include "../vmobjects/VMSymbol.h"
#include "../vmobjects/VMObject.h"
#include "../vmobjects/VMClass.h"

#include <vm/Universe.h>

ClassGenerationContext::ClassGenerationContext() :
        instanceFields(), instanceMethods(), classFields(), classMethods() {
    name = nullptr;
    superName = nullptr;
    classSide = false;
}

ClassGenerationContext::~ClassGenerationContext() {
}

void ClassGenerationContext::AddClassField(VMSymbol* field) {
    classFields.push_back(field);
}

void ClassGenerationContext::AddInstanceField(VMSymbol* field) {
    instanceFields.push_back(field);
}

void ClassGenerationContext::SetInstanceFieldsOfSuper(VMArray* fields) {
    long num = fields->GetNumberOfIndexableFields();
    for (long i = 0; i < num; i ++) {
        VMSymbol* fieldName = (VMSymbol*)fields->GetIndexableField(i);
        instanceFields.push_back(fieldName);
    }
}

void ClassGenerationContext::SetClassFieldsOfSuper(VMArray* fields) {
    long num = fields->GetNumberOfIndexableFields();
    for (long i = 0; i < num; i ++) {
        VMSymbol* fieldName = (VMSymbol*)fields->GetIndexableField(i);
        classFields.push_back(fieldName);
    }
}

bool ClassGenerationContext::HasField(VMSymbol* field) {
    if (IsClassSide()) {
        return Contains(classFields, field);
    } else {
        auto it = find(instanceFields.begin(), instanceFields.end(), field);
        return it != instanceFields.end();
    }
}

int16_t ClassGenerationContext::GetFieldIndex(VMSymbol* field) {
    if (IsClassSide()) {
        return IndexOf(classFields, field);
    } else {
        return IndexOf(instanceFields, field);
    }
}

void ClassGenerationContext::AddInstanceMethod(VMInvokable* method) {
    instanceMethods.push_back(method);
}

void ClassGenerationContext::AddClassMethod(VMInvokable* method) {
    classMethods.push_back(method);
}

VMClass* ClassGenerationContext::Assemble() {
    // build class class name
    StdString ccname = string(name->GetStdString()) + " class";

    // Load the super class
    VMClass* superClass = GetUniverse()->LoadClass(superName);

    // Allocate the class of the resulting class
    VMClass* resultClass = GetUniverse()->NewClass(load_ptr(metaClassClass));

    // Initialize the class of the resulting class
    resultClass->SetInstanceFields(GetUniverse()->NewArrayList(classFields));
    resultClass->SetInstanceInvokables(GetUniverse()->NewArrayList(classMethods));
    resultClass->SetName(GetUniverse()->SymbolFor(ccname));

    VMClass* superMClass = superClass->GetClass();
    resultClass->SetSuperClass(superMClass);

    // Allocate the resulting class
    VMClass* result = GetUniverse()->NewClass(resultClass);

    // Initialize the resulting class
    result->SetInstanceFields(GetUniverse()->NewArrayList(instanceFields));
    result->SetInstanceInvokables(GetUniverse()->NewArrayList(instanceMethods));
    result->SetName(name);
    result->SetSuperClass(superClass);

    return result;
}

void ClassGenerationContext::AssembleSystemClass(VMClass* systemClass) {
    systemClass->SetInstanceInvokables(GetUniverse()->NewArrayList
    (instanceMethods));
    systemClass->SetInstanceFields(GetUniverse()->NewArrayList(instanceFields));
    // class-bound == class-instance-bound 
        VMClass* superMClass = systemClass->GetClass();
        superMClass->SetInstanceInvokables(GetUniverse()->NewArrayList(classMethods));
        superMClass->SetInstanceFields(GetUniverse()->NewArrayList(classFields));
    }

