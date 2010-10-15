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

#include <vm/Universe.h>
 
#include "../primitivesCore/Routine.h"
#include "Object.h"

_Object::_Object( ) : PrimitiveContainer() {
    this->SetPrimitive("equalequal", new 
        Routine<_Object>(this, &_Object::Equalequal));

    this->SetPrimitive("objectSize", new 
        Routine<_Object>(this, &_Object::ObjectSize));

    this->SetPrimitive("hashcode", new 
        Routine<_Object>(this, &_Object::Hashcode));
}

void  _Object::Equalequal(pVMObject /*object*/, pVMFrame frame) {
    pVMObject op1 = frame->Pop();
    pVMObject op2 = frame->Pop();
    
    frame->Push( op1 == op2 ? trueObject : falseObject );
}


void  _Object::ObjectSize(pVMObject /*object*/, pVMFrame frame) {
    pVMObject self = frame->Pop();

    frame->Push( (pVMObject)_UNIVERSE->NewInteger(self->GetObjectSize()) );
}


void  _Object::Hashcode(pVMObject /*object*/, pVMFrame frame) {
    pVMObject self = frame->Pop();
    frame->Push( (pVMObject)_UNIVERSE->NewInteger(self->GetHash()) );
}

