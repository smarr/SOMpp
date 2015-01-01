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
    SetPrimitive("equalequal", new Routine<_Object>(this, &_Object::Equalequal));
    SetPrimitive("objectSize", new Routine<_Object>(this, &_Object::ObjectSize));
    SetPrimitive("hashcode",   new Routine<_Object>(this, &_Object::Hashcode));
    SetPrimitive("inspect",    new Routine<_Object>(this, &_Object::Inspect));
    SetPrimitive("halt",       new Routine<_Object>(this, &_Object::Halt));

    SetPrimitive("perform_",   new Routine<_Object>(this, &_Object::Perform));
    SetPrimitive("perform_withArguments_", new Routine<_Object>(this, &_Object::PerformWithArguments));
    SetPrimitive("perform_inSuperclass_",  new Routine<_Object>(this, &_Object::PerformInSuperclass));
    SetPrimitive("perform_withArguments_inSuperclass_", new Routine<_Object>(this, &_Object::PerformWithArgumentsInSuperclass));

    SetPrimitive("instVarAt_",     new Routine<_Object>(this, &_Object::InstVarAt));
    SetPrimitive("instVarAt_put_", new Routine<_Object>(this, &_Object::InstVarAtPut));
    SetPrimitive("instVarNamed_",  new Routine<_Object>(this, &_Object::InstVarNamed));
    
    SetPrimitive("class", new Routine<_Object>(this, &_Object::Class));
}

void _Object::Equalequal(VMObject* /*object*/, VMFrame* frame) {
    oop_t op1 = frame->Pop();
    oop_t op2 = frame->Pop();

    frame->Push( op1 == op2 ? trueObject : falseObject );
}

void _Object::ObjectSize(VMObject* /*object*/, VMFrame* frame) {
    oop_t self = frame->Pop();

    frame->Push(NEW_INT(AS_OBJ(self)->GetObjectSize()));
}

void _Object::Hashcode(VMObject* /*object*/, VMFrame* frame) {
    oop_t self = frame->Pop();

    if (IS_TAGGED(self))
        frame->Push(self);
    else
        frame->Push(NEW_INT(AS_OBJ(self)->GetHash()));
}

void _Object::Inspect(VMObject*, VMFrame* frame) {
    // not implemeted
    frame->Pop();
    frame->Push(falseObject);
}

void _Object::Halt(VMObject*, VMFrame* frame) {
    // not implemeted
    frame->Pop();
    frame->Push(falseObject);
}

void _Object::Perform(VMObject*, VMFrame* frame) {
    VMSymbol* selector = (VMSymbol*)frame->Pop();
    oop_t self = frame->GetStackElement(0);

    VMClass* clazz = CLASS_OF(self);
    VMInvokable* invokable = clazz->LookupInvokable(selector);

    (*invokable)(frame);
}

void _Object::PerformInSuperclass(VMObject* object, VMFrame* frame) {
    VMClass* clazz = (VMClass*) frame->Pop();
    VMSymbol* selector = (VMSymbol*)frame->Pop();

    VMInvokable* invokable = clazz->LookupInvokable(selector);

    (*invokable)(frame);
}

void _Object::PerformWithArguments(VMObject* object, VMFrame* frame) {
    VMArray* args = (VMArray*) frame->Pop();
    VMSymbol* selector = (VMSymbol*)frame->Pop();
    oop_t self = frame->GetStackElement(0);

    size_t num_args = args->GetNumberOfIndexableFields();
    for (size_t i = 0; i < num_args; i++) {
        oop_t arg = args->GetIndexableField(i);
        frame->Push(arg);
    }

    VMClass* clazz = CLASS_OF(self);
    VMInvokable* invokable = clazz->LookupInvokable(selector);

    (*invokable)(frame);
}

void _Object::PerformWithArgumentsInSuperclass(VMObject* object, VMFrame* frame) {
    VMClass* clazz = (VMClass*) frame->Pop();
    VMArray* args = (VMArray*) frame->Pop();
    VMSymbol* selector = (VMSymbol*)frame->Pop();

    size_t num_args = args->GetNumberOfIndexableFields();
    for (size_t i = 0; i < num_args; i++) {
        oop_t arg = args->GetIndexableField(i);
        frame->Push(arg);
    }

    VMInvokable* invokable = clazz->LookupInvokable(selector);

    (*invokable)(frame);
}

void _Object::InstVarAt(VMObject* object, VMFrame* frame) {
    oop_t idx = frame->Pop();
    oop_t self = frame->Pop();

    long field_idx = INT_VAL(idx) - 1;
    oop_t value = static_cast<VMObject*>(self)->GetField(field_idx);

    frame->Push(value);
}

void _Object::InstVarAtPut(VMObject* object, VMFrame* frame) {
    oop_t value = frame->Pop();
    oop_t idx   = frame->Pop();
    oop_t self  = frame->GetStackElement(0);

    long field_idx = INT_VAL(idx) - 1;

    static_cast<VMObject*>(self)->SetField(field_idx, value);
}

void _Object::InstVarNamed(VMObject* object, VMFrame* frame) {
    VMSymbol* name = (VMSymbol*) frame->Pop();
    oop_t self = frame->Pop();

    long field_idx = AS_OBJ(self)->GetFieldIndex(name);
    oop_t value = static_cast<VMObject*>(self)->GetField(field_idx);

    frame->Push(value);
}

void _Object::Class(VMObject* object, VMFrame* frame) {
    oop_t self = frame->Pop();
    frame->Push(CLASS_OF(self));
}
