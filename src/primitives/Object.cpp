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
#ifdef USE_TAGGING
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

void _Object::Equalequal(VMObject* /*object*/, VMFrame* frame) {
    VMObject* op1 = frame->Pop();
    VMObject* op2 = frame->Pop();
    
    frame->Push( op1 == op2 ? READBARRIER(trueObject) : READBARRIER(falseObject) );
}

void _Object::ObjectSize(VMObject* /*object*/, VMFrame* frame) {
    VMObject* self = frame->Pop();

#ifdef USE_TAGGING
    if IS_TAGGED(self)
    frame->Push(TAG_INTEGER(GlobalBox::IntegerBox()->GetObjectSize()));
    else
    frame->Push(TAG_INTEGER(AS_POINTER(self)->GetObjectSize()));
#else
    frame->Push(_UNIVERSE->NewInteger(self->GetObjectSize()));
#endif
}

void _Object::Hashcode(VMObject* /*object*/, VMFrame* frame) {
    VMObject* self = frame->Pop();
#ifdef USE_TAGGING
    if (IS_TAGGED(self))
    frame->Push(self);
    else
    frame->Push(TAG_INTEGER(AS_POINTER(self)->GetHash()));
#else
    frame->Push(_UNIVERSE->NewInteger(self->GetHash()));
#endif
}

void _Object::Inspect(VMObject*, VMFrame* frame) {
    // not implemeted
    frame->Pop();
    frame->Push(READBARRIER(falseObject));
}

void _Object::Halt(VMObject*, VMFrame* frame) {
    // not implemeted
    frame->Pop();
    frame->Push(READBARRIER(falseObject));
}

void _Object::Perform(VMObject*, VMFrame* frame) {
    VMSymbol* selector = (VMSymbol*)frame->Pop();
    VMObject* self = frame->GetStackElement(0);

    VMClass* clazz = self->GetClass();
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
    VMObject* self = frame->GetStackElement(0);

    size_t num_args = args->GetNumberOfIndexableFields();
    for (size_t i = 0; i < num_args; i++) {
        VMObject* arg = args->GetIndexableField(i);
        frame->Push(arg);
    }

    VMClass* clazz = self->GetClass();
    VMInvokable* invokable = clazz->LookupInvokable(selector);

    (*invokable)(frame);
}

void _Object::PerformWithArgumentsInSuperclass(VMObject* object, VMFrame* frame) {
    VMClass* clazz = (VMClass*) frame->Pop();
    VMArray* args = (VMArray*) frame->Pop();
    VMSymbol* selector = (VMSymbol*)frame->Pop();

    size_t num_args = args->GetNumberOfIndexableFields();
    for (size_t i = 0; i < num_args; i++) {
        VMObject* arg = args->GetIndexableField(i);
        frame->Push(arg);
    }

    VMInvokable* invokable = clazz->LookupInvokable(selector);

    (*invokable)(frame);
}

void _Object::InstVarAt(VMObject* object, VMFrame* frame) {
    VMInteger* idx = (VMInteger*) frame->Pop();
    VMObject* self = frame->Pop();

    long field_idx = idx->GetEmbeddedInteger() - 1;
    VMObject* value = static_cast<VMObject*>(self)->GetField(field_idx);

    frame->Push(value);
}

void _Object::InstVarAtPut(VMObject* object, VMFrame* frame) {
    VMObject* value = frame->Pop();
    VMInteger* idx = (VMInteger*) frame->Pop();
    VMObject* self = frame->GetStackElement(0);

    long field_idx = idx->GetEmbeddedInteger() - 1;

    static_cast<VMObject*>(self)->SetField(field_idx, value);
}

void _Object::InstVarNamed(VMObject* object, VMFrame* frame) {
    VMSymbol* name = (VMSymbol*) frame->Pop();
    VMObject* self = frame->Pop();

    long field_idx = self->GetFieldIndex(name);
    VMObject* value = static_cast<VMObject*>(self)->GetField(field_idx);

    frame->Push(value);
}

void _Object::Class(VMObject* object, VMFrame* frame) {
    VMObject* self = frame->Pop();
    frame->Push(self->GetClass());
}
