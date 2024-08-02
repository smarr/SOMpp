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

#include "../primitivesCore/PrimitiveContainer.h"
#include "../primitivesCore/Routine.h"
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
    } else {
        return NEW_INT(AS_OBJ(self)->GetHash());
    }
}

vm_oop_t objInspect(vm_oop_t) {
    // not implemeted
    return load_ptr(falseObject);
}

vm_oop_t objHalt(vm_oop_t) {
    // not implemeted
    return load_ptr(falseObject);
}

void _Object::Perform(Interpreter* interp, VMFrame* frame) {
    VMSymbol* selector = (VMSymbol*)frame->Pop();
    vm_oop_t self = frame->GetStackElement(0);

    VMClass* clazz = CLASS_OF(self);
    VMInvokable* invokable = clazz->LookupInvokable(selector);

    invokable->Invoke(interp, frame);
}

void _Object::PerformInSuperclass(Interpreter* interp, VMFrame* frame) {
    VMClass* clazz = (VMClass*)frame->Pop();
    VMSymbol* selector = (VMSymbol*)frame->Pop();

    VMInvokable* invokable = clazz->LookupInvokable(selector);

    invokable->Invoke(interp, frame);
}

void _Object::PerformWithArguments(Interpreter* interp, VMFrame* frame) {
    VMArray* args = (VMArray*)frame->Pop();
    VMSymbol* selector = (VMSymbol*)frame->Pop();
    vm_oop_t self = frame->GetStackElement(0);

    size_t num_args = args->GetNumberOfIndexableFields();
    for (size_t i = 0; i < num_args; i++) {
        vm_oop_t arg = args->GetIndexableField(i);
        frame->Push(arg);
    }

    VMClass* clazz = CLASS_OF(self);
    VMInvokable* invokable = clazz->LookupInvokable(selector);

    invokable->Invoke(interp, frame);
}

void _Object::PerformWithArgumentsInSuperclass(Interpreter* interp,
                                               VMFrame* frame) {
    VMClass* clazz = (VMClass*)frame->Pop();
    VMArray* args = (VMArray*)frame->Pop();
    VMSymbol* selector = (VMSymbol*)frame->Pop();

    size_t num_args = args->GetNumberOfIndexableFields();
    for (size_t i = 0; i < num_args; i++) {
        vm_oop_t arg = args->GetIndexableField(i);
        frame->Push(arg);
    }

    VMInvokable* invokable = clazz->LookupInvokable(selector);

    invokable->Invoke(interp, frame);
}

vm_oop_t objInstVarAt(vm_oop_t self, vm_oop_t idx) {
    int64_t field_idx = INT_VAL(idx) - 1;
    return static_cast<VMObject*>(self)->GetField(field_idx);
}

void _Object::InstVarAtPut(Interpreter*, VMFrame* frame) {
    vm_oop_t value = frame->Pop();
    vm_oop_t idx = frame->Pop();
    vm_oop_t self = frame->GetStackElement(0);

    long field_idx = INT_VAL(idx) - 1;

    static_cast<VMObject*>(self)->SetField(field_idx, value);
}

vm_oop_t objInstVarNamed(vm_oop_t self, vm_oop_t nameObj) {
    VMSymbol* name = (VMSymbol*)nameObj;
    long field_idx = AS_OBJ(self)->GetFieldIndex(name);
    return static_cast<VMObject*>(self)->GetField(field_idx);
}

vm_oop_t objClass(vm_oop_t self) {
    return CLASS_OF(self);
}

_Object::_Object() : PrimitiveContainer() {
    Add("equalequal", &objEqualequal, false);
    Add("objectSize", &objObjectSize, false);
    Add("hashcode", &objHashcode, false);
    Add("inspect", &objInspect, false);
    Add("halt", &objHalt, false);

    SetPrimitive("perform_",
                 new Routine<_Object>(this, &_Object::Perform, false));
    SetPrimitive(
        "perform_withArguments_",
        new Routine<_Object>(this, &_Object::PerformWithArguments, false));
    SetPrimitive(
        "perform_inSuperclass_",
        new Routine<_Object>(this, &_Object::PerformInSuperclass, false));
    SetPrimitive("perform_withArguments_inSuperclass_",
                 new Routine<_Object>(
                     this, &_Object::PerformWithArgumentsInSuperclass, false));

    Add("instVarAt_", &objInstVarAt, false);
    SetPrimitive("instVarAt_put_",
                 new Routine<_Object>(this, &_Object::InstVarAtPut, false));
    Add("instVarNamed_", &objInstVarNamed, false);

    Add("class", &objClass, false);
}
