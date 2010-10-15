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

_Array::_Array() : PrimitiveContainer()
{
    this->SetPrimitive("new_", static_cast<PrimitiveRoutine*>(
                        new Routine<_Array>(this, &_Array::New_)));
    this->SetPrimitive("at_", static_cast<PrimitiveRoutine*>(
                        new Routine<_Array>(this, &_Array::At_)));
    this->SetPrimitive("at_put_", static_cast<PrimitiveRoutine*>(
                        new Routine<_Array>(this, &_Array::At_Put_)));
    this->SetPrimitive("length",  static_cast<PrimitiveRoutine*>(
                        new Routine<_Array>(this, &_Array::Length)));
}

void _Array::At_(pVMObject /*object*/, pVMFrame frame) {
    pVMInteger index = (pVMInteger) frame->Pop();
    pVMArray self = (pVMArray) frame->Pop();
    int i = index->GetEmbeddedInteger();
    pVMObject elem = (*self)[i-1];
    frame->Push(elem);
}


void _Array::At_Put_(pVMObject /*object*/, pVMFrame frame) {
    pVMObject value = frame->Pop();
    pVMInteger index = (pVMInteger)frame->Pop();
    pVMArray self = (pVMArray)frame->GetStackElement(0);
    int i = index->GetEmbeddedInteger();
    (*self)[i - 1] = value;
}


void _Array::Length(pVMObject /*object*/, pVMFrame frame) {
    pVMArray self = (pVMArray) frame->Pop();
    pVMInteger new_int =
        _UNIVERSE->NewInteger(self->GetNumberOfIndexableFields());
    frame->Push((pVMObject)new_int);
}


void _Array::New_(pVMObject /*object*/, pVMFrame frame) {
    pVMInteger length = (pVMInteger)frame->Pop();
    /*pVMClass self = (pVMClass)*/
    frame->Pop();        
    int size = length->GetEmbeddedInteger();
    frame->Push((pVMObject) _UNIVERSE->NewArray(size));
}

