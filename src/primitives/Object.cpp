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

#include <vmobjects/VMObject.h>
#include <vmobjects/VMFrame.h>
#include <vmobjects/VMClass.h>
#include <vmobjects/VMInvokable.h>

#include <vm/Universe.h>
#include "../vmobjects/IntegerBox.h"
#include "../primitivesCore/Routine.h"
#include "Object.h"

_Object::_Object() : PrimitiveContainer() {
    SetPrimitive("equalequal", new Routine<_Object>(this, &_Object::Equalequal, false));
    SetPrimitive("objectSize", new Routine<_Object>(this, &_Object::ObjectSize, false));
    SetPrimitive("hashcode",   new Routine<_Object>(this, &_Object::Hashcode,   false));
    SetPrimitive("inspect",    new Routine<_Object>(this, &_Object::Inspect,    false));
    SetPrimitive("halt",       new Routine<_Object>(this, &_Object::Halt,       false));

    SetPrimitive("perform_",   new Routine<_Object>(this, &_Object::Perform, false));
    SetPrimitive("perform_withArguments_", new Routine<_Object>(this, &_Object::PerformWithArguments, false));
    SetPrimitive("perform_inSuperclass_",  new Routine<_Object>(this, &_Object::PerformInSuperclass, false));
    SetPrimitive("perform_withArguments_inSuperclass_", new Routine<_Object>(this, &_Object::PerformWithArgumentsInSuperclass, false));

    SetPrimitive("instVarAt_",     new Routine<_Object>(this, &_Object::InstVarAt, false));
    SetPrimitive("instVarAt_put_", new Routine<_Object>(this, &_Object::InstVarAtPut, false));
    SetPrimitive("instVarNamed_",  new Routine<_Object>(this, &_Object::InstVarNamed, false));
    
    SetPrimitive("class", new Routine<_Object>(this, &_Object::Class, false));
}

void _Object::Equalequal(Interpreter*, VMFrame* frame) {
    vm_oop_t op1 = frame->Pop();
    vm_oop_t op2 = frame->Pop();

    frame->Push(load_ptr(op1 == op2 ? trueObject : falseObject));
}

void _Object::ObjectSize(Interpreter*, VMFrame* frame) {
    vm_oop_t self = frame->Pop();

    frame->Push(NEW_INT(AS_OBJ(self)->GetObjectSize()));
}

void _Object::Hashcode(Interpreter*, VMFrame* frame) {
    vm_oop_t self = frame->Pop();

    if (IS_TAGGED(self))
        frame->Push(self);
    else
        frame->Push(NEW_INT(AS_OBJ(self)->GetHash()));
}

void _Object::Inspect(Interpreter*, VMFrame* frame) {
    // not implemeted
    frame->Pop();
    frame->Push(load_ptr(falseObject));
}

void _Object::Halt(Interpreter*, VMFrame* frame) {
    // not implemeted
    frame->Pop();
    frame->Push(load_ptr(falseObject));
}

void _Object::Perform(Interpreter* interp, VMFrame* frame) {
    VMSymbol* selector = (VMSymbol*)frame->Pop();
    vm_oop_t self = frame->GetStackElement(0);

    VMClass* clazz = CLASS_OF(self);
    VMInvokable* invokable = clazz->LookupInvokable(selector);

    invokable->Invoke(interp, frame);
}

void _Object::PerformInSuperclass(Interpreter* interp, VMFrame* frame) {
    VMClass* clazz = (VMClass*) frame->Pop();
    VMSymbol* selector = (VMSymbol*)frame->Pop();

    VMInvokable* invokable = clazz->LookupInvokable(selector);

    invokable->Invoke(interp, frame);
}

void _Object::PerformWithArguments(Interpreter* interp, VMFrame* frame) {
    VMArray* args = (VMArray*) frame->Pop();
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

void _Object::PerformWithArgumentsInSuperclass(Interpreter* interp, VMFrame* frame) {
    VMClass* clazz = (VMClass*) frame->Pop();
    VMArray* args = (VMArray*) frame->Pop();
    VMSymbol* selector = (VMSymbol*)frame->Pop();

    size_t num_args = args->GetNumberOfIndexableFields();
    for (size_t i = 0; i < num_args; i++) {
        vm_oop_t arg = args->GetIndexableField(i);
        frame->Push(arg);
    }

    VMInvokable* invokable = clazz->LookupInvokable(selector);

    invokable->Invoke(interp, frame);
}

void _Object::InstVarAt(Interpreter*, VMFrame* frame) {
    vm_oop_t idx = frame->Pop();
    vm_oop_t self = frame->Pop();

    long field_idx = INT_VAL(idx) - 1;
    vm_oop_t value = static_cast<VMObject*>(self)->GetField(field_idx);

    frame->Push(value);
}

void _Object::InstVarAtPut(Interpreter*, VMFrame* frame) {
    vm_oop_t value = frame->Pop();
    vm_oop_t idx   = frame->Pop();
    vm_oop_t self  = frame->GetStackElement(0);

    long field_idx = INT_VAL(idx) - 1;

    static_cast<VMObject*>(self)->SetField(field_idx, value);
}

void _Object::InstVarNamed(Interpreter*, VMFrame* frame) {
    VMSymbol* name = (VMSymbol*) frame->Pop();
    vm_oop_t self = frame->Pop();

    long field_idx = AS_OBJ(self)->GetFieldIndex(name);
    vm_oop_t value = static_cast<VMObject*>(self)->GetField(field_idx);

    frame->Push(value);
}

void _Object::Class(Interpreter*, VMFrame* frame) {
    vm_oop_t self = frame->Pop();
    frame->Push(CLASS_OF(self));
}
