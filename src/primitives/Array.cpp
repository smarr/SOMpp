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

#include "Array.h"

#include "../primitivesCore/Routine.h"

#include <vmobjects/VMInteger.h>
#include <vmobjects/VMArray.h>
#include <vmobjects/VMObject.h>
#include <vmobjects/VMFrame.h>

#include <vm/Universe.h>

_Array::_Array() :
        PrimitiveContainer() {
    this->SetPrimitive("new_",
            static_cast<PrimitiveRoutine*>(new Routine<_Array>(this,
                    &_Array::New_)));
    this->SetPrimitive("at_",
            static_cast<PrimitiveRoutine*>(new Routine<_Array>(this,
                    &_Array::At_)));
    this->SetPrimitive("at_put_",
            static_cast<PrimitiveRoutine*>(new Routine<_Array>(this,
                    &_Array::At_Put_)));
    this->SetPrimitive("length",
            static_cast<PrimitiveRoutine*>(new Routine<_Array>(this,
                    &_Array::Length)));
}

void _Array::At_(VMObject* /*object*/, VMFrame* frame) {
    VMInteger* index = static_cast<VMInteger*>(frame->Pop());
    VMArray* self = static_cast<VMArray*>(frame->Pop());
#ifdef USE_TAGGING
    long i = UNTAG_INTEGER(index);
#else
    long i = index->GetEmbeddedInteger();
#endif
    VMObject* elem = self->GetIndexableField(i - 1);
    frame->Push(elem);
}

void _Array::At_Put_(VMObject* /*object*/, VMFrame* frame) {
    VMObject* value = frame->Pop();
    VMInteger* index = static_cast<VMInteger*>(frame->Pop());
    VMArray* self = static_cast<VMArray*>(frame->GetStackElement(0));
#ifdef USE_TAGGING
    long i = UNTAG_INTEGER(index);
#else
    long i = index->GetEmbeddedInteger();
#endif
    self->SetIndexableField(i - 1, value);
}

void _Array::Length(VMObject* /*object*/, VMFrame* frame) {
    VMArray* self = static_cast<VMArray*>(frame->Pop());
#ifdef USE_TAGGING
    VMInteger* new_int = TAG_INTEGER(self->GetNumberOfIndexableFields());
#else
    pVMInteger new_int = _UNIVERSE->NewInteger(self->GetNumberOfIndexableFields());
#endif
    frame->Push(new_int);
}

void _Array::New_(VMObject* /*object*/, VMFrame* frame) {
    VMInteger* length = static_cast<VMInteger*>(frame->Pop());
    frame->Pop();
#ifdef USE_TAGGING
    long size = UNTAG_INTEGER(length);
#else
    long size = length->GetEmbeddedInteger();
#endif
    frame->Push(_UNIVERSE->NewArray(size));
}

