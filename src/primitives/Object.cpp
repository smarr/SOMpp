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

#include "Object.h"

#include <cstddef>
#include <cstdint>

#include "../vm/Globals.h"
#include "../vm/Universe.h"  // NOLINT(misc-include-cleaner) it's required to make the types complete
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMArray.h"
#include "../vmobjects/VMClass.h"  // NOLINT(misc-include-cleaner) it's required to make the types complete
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMInvokable.h"
#include "../vmobjects/VMObject.h"

vm_oop_t objEqualequal(vm_oop_t op2, vm_oop_t op1) {
    return load_ptr(op1 == op2 ? trueObject : falseObject);
}

vm_oop_t objObjectSize(vm_oop_t self) {
    return NEW_INT((int64_t)AS_OBJ(self)->GetObjectSize());
}

vm_oop_t objHashcode(vm_oop_t self) {
    if (IS_TAGGED(self)) {
        return self;
    }
    return NEW_INT(AS_OBJ(self)->GetHash());
}

vm_oop_t objInspect(vm_oop_t /*unused*/) {
    // not implemeted
    return load_ptr(falseObject);
}

vm_oop_t objHalt(vm_oop_t /*unused*/) {
    // not implemeted
    return load_ptr(falseObject);
}

void objPerform(VMFrame* frame) {
    auto* selector = (VMSymbol*)frame->Pop();
    vm_oop_t self = frame->GetStackElement(0);

    VMClass* clazz = CLASS_OF(self);
    VMInvokable* invokable = clazz->LookupInvokable(selector);

    invokable->Invoke(frame);
}

void objPerformInSuperclass(VMFrame* frame) {
    auto* clazz = (VMClass*)frame->Pop();
    auto* selector = (VMSymbol*)frame->Pop();

    VMInvokable* invokable = clazz->LookupInvokable(selector);

    invokable->Invoke(frame);
}

void objPerformWithArguments(VMFrame* frame) {
    auto* args = (VMArray*)frame->Pop();
    auto* selector = (VMSymbol*)frame->Pop();
    vm_oop_t self = frame->GetStackElement(0);

    size_t const num_args = args->GetNumberOfIndexableFields();
    for (size_t i = 0; i < num_args; i++) {
        vm_oop_t arg = args->GetIndexableField(i);
        frame->Push(arg);
    }

    VMClass* clazz = CLASS_OF(self);
    VMInvokable* invokable = clazz->LookupInvokable(selector);

    invokable->Invoke(frame);
}

void objPerformWithArgumentsInSuperclass(VMFrame* frame) {
    auto* clazz = (VMClass*)frame->Pop();
    auto* args = (VMArray*)frame->Pop();
    auto* selector = (VMSymbol*)frame->Pop();

    size_t const num_args = args->GetNumberOfIndexableFields();
    for (size_t i = 0; i < num_args; i++) {
        vm_oop_t arg = args->GetIndexableField(i);
        frame->Push(arg);
    }

    VMInvokable* invokable = clazz->LookupInvokable(selector);

    invokable->Invoke(frame);
}

vm_oop_t objInstVarAt(vm_oop_t self, vm_oop_t idx) {
    int64_t const field_idx = INT_VAL(idx) - 1;
    return static_cast<VMObject*>(self)->GetField(field_idx);
}

vm_oop_t objInstVarAtPut(vm_oop_t self, vm_oop_t idx, vm_oop_t value) {
    size_t const field_idx = INT_VAL(idx) - 1;
    static_cast<VMObject*>(self)->SetField(field_idx, value);
    return self;
}

vm_oop_t objInstVarNamed(vm_oop_t self, vm_oop_t nameObj) {
    auto* name = (VMSymbol*)nameObj;
    int64_t const fieldIdx = AS_OBJ(self)->GetFieldIndex(name);
    return static_cast<VMObject*>(self)->GetField(fieldIdx);
}

vm_oop_t objClass(vm_oop_t self) {
    return CLASS_OF(self);
}

_Object::_Object() {
    Add("==", &objEqualequal, false);
    Add("objectSize", &objObjectSize, false);
    Add("hashcode", &objHashcode, false);
    Add("inspect", &objInspect, false);
    Add("halt", &objHalt, false);

    Add("perform:", &objPerform, false);
    Add("perform:withArguments:", &objPerformWithArguments, false);
    Add("perform:inSuperclass:", &objPerformInSuperclass, false);
    Add("perform:withArguments:inSuperclass:",
        &objPerformWithArgumentsInSuperclass, false);

    Add("instVarAt:", &objInstVarAt, false);
    Add("instVarAt:put:", &objInstVarAtPut, false);
    Add("instVarNamed:", &objInstVarNamed, false);

    Add("class", &objClass, false);
}
