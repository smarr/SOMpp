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

void _Array::At_(pVMObject /*object*/, pVMFrame frame) {
    pVMInteger index = static_cast<pVMInteger>(frame->Pop());
    pVMArray self = static_cast<pVMArray>(frame->Pop());
    long i = INT_VAL(index);
    oop_t elem = self->GetIndexableField(i - 1);
    frame->Push(elem);
}

void _Array::At_Put_(pVMObject /*object*/, pVMFrame frame) {
    oop_t value = frame->Pop();
    pVMInteger index = static_cast<pVMInteger>(frame->Pop());
    pVMArray self = static_cast<pVMArray>(frame->GetStackElement(0));
    long i = INT_VAL(index);
    self->SetIndexableField(i - 1, value);
}

void _Array::Length(pVMObject /*object*/, pVMFrame frame) {
    pVMArray self = static_cast<pVMArray>(frame->Pop());
    pVMInteger new_int = NEW_INT(self->GetNumberOfIndexableFields());
    frame->Push(new_int);
}

void _Array::New_(pVMObject /*object*/, pVMFrame frame) {
    pVMInteger length = static_cast<pVMInteger>(frame->Pop());
    frame->Pop();
    long size = INT_VAL(length);
    frame->Push(GetUniverse()->NewArray(size));
}

