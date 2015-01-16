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

#include <stdio.h>

#include <vmobjects/VMObject.h>
#include <vmobjects/VMFrame.h>
#include <vmobjects/VMSymbol.h>
#include <vmobjects/VMInteger.h>

#include <vm/Universe.h>

#include <misc/defs.h>

#include "String.h"
#include "../primitivesCore/Routine.h"

_String::_String() : PrimitiveContainer() {
    SetPrimitive("concatenate_", new Routine<_String>(this, &_String::Concatenate_));
    SetPrimitive("asSymbol",     new Routine<_String>(this, &_String::AsSymbol));
    SetPrimitive("hashcode",     new Routine<_String>(this, &_String::Hashcode));
    SetPrimitive("length",       new Routine<_String>(this, &_String::Length));
    SetPrimitive("equal",        new Routine<_String>(this, &_String::Equal));
    SetPrimitive("primSubstringFrom_to_",
            new Routine<_String>(this, &_String::PrimSubstringFrom_to_));
}

void _String::Concatenate_(VMObject* /*object*/, VMFrame* frame) {
    VMString* arg  = static_cast<VMString*>(frame->Pop());
    VMString* self = static_cast<VMString*>(frame->Pop());
    StdString a = arg->GetChars();
    StdString s = self->GetChars();

    StdString result = s + a;

    frame->Push(GetUniverse()->NewString(result));
}

void _String::AsSymbol(VMObject* /*object*/, VMFrame* frame) {
    VMString* self = static_cast<VMString*>(frame->Pop());
    StdString result = self->GetStdString();
    frame->Push(GetUniverse()->SymbolFor(result));
}

void _String::Hashcode(VMObject* /*object*/, VMFrame* frame) {
    VMString* self = static_cast<VMString*>(frame->Pop());
#ifdef USE_TAGGING
    frame->Push(TAG_INTEGER(AS_POINTER(self)->GetHash()));
#else
    frame->Push(GetUniverse()->NewInteger(self->GetHash()));
#endif
}

void _String::Length(VMObject* /*object*/, VMFrame* frame) {
    VMString* self = static_cast<VMString*>(frame->Pop());

    size_t len = self->GetStringLength();
#ifdef USE_TAGGING
    frame->Push(TAG_INTEGER(len));
#else
    frame->Push(GetUniverse()->NewInteger((long)len));
#endif
}

void _String::Equal(VMObject* /*object*/, VMFrame* frame) {
    VMObject* op1 = frame->Pop();
    VMString* op2 = static_cast<VMString*>(frame->Pop());

#ifdef USE_TAGGING
    if (IS_TAGGED(op1)) {
        frame->Push(falseObject);
        return;
    }
    VMClass* otherClass = AS_POINTER(op1)->GetClass();
#else
    VMClass* otherClass = op1->GetClass();
#endif
    if(otherClass == READBARRIER(stringClass)) {

        StdString s1 = static_cast<VMString*>(op1)->GetStdString();
        StdString s2 = op2->GetStdString();

        if(s1 == s2) {
            frame->Push(READBARRIER(trueObject));
            return;
        }
    }
    frame->Push(READBARRIER(falseObject));
}

void _String::PrimSubstringFrom_to_(VMObject* /*object*/, VMFrame* frame) {
    VMInteger* end = static_cast<VMInteger*>(frame->Pop());
    VMInteger* start = static_cast<VMInteger*>(frame->Pop());

    VMString* self = static_cast<VMString*>(frame->Pop());
    StdString str = self->GetStdString();
#ifdef USE_TAGGING
    long s = UNTAG_INTEGER(start) - 1;
    long e = UNTAG_INTEGER(end) - 1;
#else
    long s = start->GetEmbeddedInteger() - 1;
    long e = end->GetEmbeddedInteger() - 1;
#endif

    StdString result = str.substr(s, e - s + 1);

    frame->Push( GetUniverse()->NewString(result));
}

