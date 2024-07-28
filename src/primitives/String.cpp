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

#include <cctype>
#include <cstdint>
#include <cstdio>

#include "../misc/defs.h"
#include "../primitivesCore/PrimitiveContainer.h"
#include "../primitivesCore/Routine.h"
#include "../vm/Globals.h"
#include "../vm/Symbols.h"
#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMString.h"
#include "../vmobjects/VMSymbol.h" // NOLINT(misc-include-cleaner) it's required to make the types complete
#include "String.h"

_String::_String() : PrimitiveContainer() {
    SetPrimitive("concatenate_", new Routine<_String>(this, &_String::Concatenate_, false));
    SetPrimitive("asSymbol",     new Routine<_String>(this, &_String::AsSymbol, false));
    SetPrimitive("hashcode",     new Routine<_String>(this, &_String::Hashcode, false));
    SetPrimitive("length",       new Routine<_String>(this, &_String::Length,   false));
    SetPrimitive("equal",        new Routine<_String>(this, &_String::Equal,    false));
    SetPrimitive("primSubstringFrom_to_", new Routine<_String>(this, &_String::PrimSubstringFrom_to_, false));
    SetPrimitive("isWhiteSpace",        new Routine<_String>(this, &_String::IsWhiteSpace, false));
    SetPrimitive("isLetters",        new Routine<_String>(this, &_String::IsLetters,    false));
    SetPrimitive("isDigits",        new Routine<_String>(this, &_String::IsDigits,     false));
}

void _String::Concatenate_(Interpreter*, VMFrame* frame) {
    VMString* arg  = static_cast<VMString*>(frame->Pop());
    VMString* self = static_cast<VMString*>(frame->Pop());
    // TODO: if this really needs to be optimized, than, well, then,
    // NewString should allow to construct it correctly and simply copy
    // from both input strings
    StdString a = arg->GetStdString();
    StdString s = self->GetStdString();

    StdString result = s + a;

    frame->Push(GetUniverse()->NewString(result));
}

void _String::AsSymbol(Interpreter*, VMFrame* frame) {
    VMString* self = static_cast<VMString*>(frame->Pop());
    StdString result = self->GetStdString();
    frame->Push(SymbolFor(result));
}

void _String::Hashcode(Interpreter*, VMFrame* frame) {
    VMString* self = static_cast<VMString*>(frame->Pop());
    frame->Push(NEW_INT(self->GetHash()));
}

void _String::Length(Interpreter*, VMFrame* frame) {
    VMString* self = static_cast<VMString*>(frame->Pop());

    size_t len = self->GetStringLength();
    frame->Push(NEW_INT((int64_t) len));
}

void _String::Equal(Interpreter*, VMFrame* frame) {
    vm_oop_t op1 = frame->Pop();
    VMString* op2 = static_cast<VMString*>(frame->Pop());

    if (IS_TAGGED(op1)) {
        frame->Push(load_ptr(falseObject));
        return;
    }

    VMClass* otherClass = CLASS_OF(op1);
    if(otherClass == load_ptr(stringClass) || otherClass == load_ptr(symbolClass)) {
        StdString s1 = static_cast<VMString*>(op1)->GetStdString();
        StdString s2 = op2->GetStdString();

        if(s1 == s2) {
            frame->Push(load_ptr(trueObject));
            return;
        }
    }
    frame->Push(load_ptr(falseObject));
}

void _String::PrimSubstringFrom_to_(Interpreter*, VMFrame* frame) {
    vm_oop_t end   = frame->Pop();
    vm_oop_t start = frame->Pop();

    VMString* self = static_cast<VMString*>(frame->Pop());
    StdString str = self->GetStdString();

    int64_t s = INT_VAL(start) - 1;
    int64_t e = INT_VAL(end) - 1;

    StdString result = str.substr(s, e - s + 1);

    frame->Push(GetUniverse()->NewString(result));
}

void _String::IsWhiteSpace(Interpreter*, VMFrame* frame) {
    VMString* self = static_cast<VMString*>(frame->Pop());

    size_t len = self->GetStringLength();

    const char* string = self->GetRawChars();

    for (size_t i = 0; i < len; i++) {
        if (!isspace(string[i])) {
            frame->Push(load_ptr(falseObject));
            return;
        }
    }

    if (len > 0) {
        frame->Push(load_ptr(trueObject));
    } else {
        frame->Push(load_ptr(falseObject));
    }
}

void _String::IsLetters(Interpreter*, VMFrame* frame) {
    VMString* self = static_cast<VMString*>(frame->Pop());

    size_t len = self->GetStringLength();

    const char* string = self->GetRawChars();

    for (size_t i = 0; i < len; i++) {
        if (!isalpha(string[i])) {
            frame->Push(load_ptr(falseObject));
            return;
        }
    }

    if (len > 0) {
        frame->Push(load_ptr(trueObject));
    } else {
        frame->Push(load_ptr(falseObject));
    }
}

void _String::IsDigits(Interpreter*, VMFrame* frame) {
    VMString* self = static_cast<VMString*>(frame->Pop());

    size_t len = self->GetStringLength();

    const char* string = self->GetRawChars();

    for (size_t i = 0; i < len; i++) {
        if (!isdigit(string[i])) {
            frame->Push(load_ptr(falseObject));
            return;
        }
    }

    if (len > 0) {
        frame->Push(load_ptr(trueObject));
    } else {
        frame->Push(load_ptr(falseObject));
    }
}
