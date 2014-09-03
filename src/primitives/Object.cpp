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
#if USE_TAGGING
#include "../vmobjects/IntegerBox.h"
#endif

#include "../primitivesCore/Routine.h"
#include "Object.h"

_Object::_Object() :
        PrimitiveContainer() {
    this->SetPrimitive("equalequal",
            new Routine<_Object>(this, &_Object::Equalequal));

    this->SetPrimitive("objectSize",
            new Routine<_Object>(this, &_Object::ObjectSize));

    this->SetPrimitive("hashcode",
            new Routine<_Object>(this, &_Object::Hashcode));

    this->SetPrimitive("inspect",
            new Routine<_Object>(this, &_Object::Inspect));

    this->SetPrimitive("halt", new Routine<_Object>(this, &_Object::Halt));

    this->SetPrimitive("perform_",
            new Routine<_Object>(this, &_Object::Perform));
    this->SetPrimitive("perform_withArguments_",
            new Routine<_Object>(this, &_Object::PerformWithArguments));
    this->SetPrimitive("perform_inSuperclass_",
            new Routine<_Object>(this, &_Object::PerformInSuperclass));
    this->SetPrimitive("perform_withArguments_inSuperclass_",
            new Routine<_Object>(this,
                    &_Object::PerformWithArgumentsInSuperclass));

    this->SetPrimitive("instVarAt_",
            new Routine<_Object>(this, &_Object::InstVarAt));
    this->SetPrimitive("instVarAt_put_",
            new Routine<_Object>(this, &_Object::InstVarAtPut));
    this->SetPrimitive("instVarNamed_",
            new Routine<_Object>(this, &_Object::InstVarNamed));
    
    this->SetPrimitive("class", new Routine<_Object>(this, &_Object::Class));
}

void _Object::Equalequal(pVMObject /*object*/, pVMFrame frame) {
    oop_t op1 = frame->Pop();
    oop_t op2 = frame->Pop();

    frame->Push( op1 == op2 ? trueObject : falseObject );
}

void _Object::ObjectSize(pVMObject /*object*/, pVMFrame frame) {
    oop_t self = frame->Pop();

#if USE_TAGGING
    if IS_TAGGED(self)
        frame->Push(TAG_INTEGER(GlobalBox::IntegerBox()->GetObjectSize()));
    else
        frame->Push(TAG_INTEGER(self->GetObjectSize()));
#else
    frame->Push(GetUniverse()->NewInteger(self->GetObjectSize()));
#endif
}

void _Object::Hashcode(pVMObject /*object*/, pVMFrame frame) {
    oop_t self = frame->Pop();

    if (IS_TAGGED(self))
        frame->Push(self);
    else
        frame->Push(TAG_INTEGER(self->GetHash()));
}

void _Object::Inspect(pVMObject, pVMFrame frame) {
    // not implemeted
    frame->Pop();
    frame->Push(falseObject);
}

void _Object::Halt(pVMObject, pVMFrame frame) {
    // not implemeted
    frame->Pop();
    frame->Push(falseObject);
}

void _Object::Perform(pVMObject, pVMFrame frame) {
    pVMSymbol selector = (pVMSymbol)frame->Pop();
    oop_t self = frame->GetStackElement(0);

    pVMClass clazz = self->GetClass();
    pVMInvokable invokable = clazz->LookupInvokable(selector);

    (*invokable)(frame);
}

void _Object::PerformInSuperclass(pVMObject object, pVMFrame frame) {
    pVMClass clazz = (pVMClass) frame->Pop();
    pVMSymbol selector = (pVMSymbol)frame->Pop();

    pVMInvokable invokable = clazz->LookupInvokable(selector);

    (*invokable)(frame);
}

void _Object::PerformWithArguments(pVMObject object, pVMFrame frame) {
    pVMArray args = (pVMArray) frame->Pop();
    pVMSymbol selector = (pVMSymbol)frame->Pop();
    oop_t self = frame->GetStackElement(0);

    size_t num_args = args->GetNumberOfIndexableFields();
    for (size_t i = 0; i < num_args; i++) {
        oop_t arg = args->GetIndexableField(i);
        frame->Push(arg);
    }

    pVMClass clazz = self->GetClass();
    pVMInvokable invokable = clazz->LookupInvokable(selector);

    (*invokable)(frame);
}

void _Object::PerformWithArgumentsInSuperclass(pVMObject object, pVMFrame frame) {
    pVMClass clazz = (pVMClass) frame->Pop();
    pVMArray args = (pVMArray) frame->Pop();
    pVMSymbol selector = (pVMSymbol)frame->Pop();

    size_t num_args = args->GetNumberOfIndexableFields();
    for (size_t i = 0; i < num_args; i++) {
        oop_t arg = args->GetIndexableField(i);
        frame->Push(arg);
    }

    pVMInvokable invokable = clazz->LookupInvokable(selector);

    (*invokable)(frame);
}

void _Object::InstVarAt(pVMObject object, pVMFrame frame) {
    pVMInteger idx = (pVMInteger) frame->Pop();
    oop_t self = frame->Pop();

    long field_idx = idx->GetEmbeddedInteger() - 1;
    oop_t value = static_cast<VMObject*>(self)->GetField(field_idx);

    frame->Push(value);
}

void _Object::InstVarAtPut(pVMObject object, pVMFrame frame) {
    oop_t value = frame->Pop();
    pVMInteger idx = (pVMInteger) frame->Pop();
    oop_t self = frame->GetStackElement(0);

    long field_idx = idx->GetEmbeddedInteger() - 1;

    static_cast<VMObject*>(self)->SetField(field_idx, value);
}

void _Object::InstVarNamed(pVMObject object, pVMFrame frame) {
    pVMSymbol name = (pVMSymbol) frame->Pop();
    oop_t self = frame->Pop();

    long field_idx = self->GetFieldIndex(name);
    oop_t value = static_cast<pVMObject>(self)->GetField(field_idx);

    frame->Push(value);
}

void _Object::Class(pVMObject object, pVMFrame frame) {
    oop_t self = frame->Pop();
    frame->Push(self->GetClass());
}
