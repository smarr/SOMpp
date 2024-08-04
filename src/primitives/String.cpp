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

#include "String.h"

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
#include "../vmobjects/VMSymbol.h"  // NOLINT(misc-include-cleaner) it's required to make the types complete

static vm_oop_t strConcatenate_(vm_oop_t leftObj, vm_oop_t rightObj) {
    VMString* arg = static_cast<VMString*>(rightObj);
    VMString* self = static_cast<VMString*>(leftObj);
    // TODO: if this really needs to be optimized, than, well, then,
    // NewString should allow to construct it correctly and simply copy
    // from both input strings
    StdString a = arg->GetStdString();
    StdString s = self->GetStdString();

    StdString result = s + a;

    return GetUniverse()->NewString(result);
}

static vm_oop_t strAsSymbol(vm_oop_t rcvr) {
    VMString* self = static_cast<VMString*>(rcvr);
    StdString result = self->GetStdString();
    return SymbolFor(result);
}

static vm_oop_t strHashcode(vm_oop_t rcvr) {
    VMString* self = static_cast<VMString*>(rcvr);
    return NEW_INT(self->GetHash());
}

static vm_oop_t strLength(vm_oop_t rcvr) {
    VMString* self = static_cast<VMString*>(rcvr);

    size_t len = self->GetStringLength();
    return NEW_INT((int64_t)len);
}

static vm_oop_t strEqual(vm_oop_t leftObj, vm_oop_t op1) {
    VMString* op2 = static_cast<VMString*>(leftObj);

    if (IS_TAGGED(op1)) {
        return load_ptr(falseObject);
    }

    VMClass* otherClass = CLASS_OF(op1);
    if (otherClass == load_ptr(stringClass) ||
        otherClass == load_ptr(symbolClass)) {
        StdString s1 = static_cast<VMString*>(op1)->GetStdString();
        StdString s2 = op2->GetStdString();

        if (s1 == s2) {
            return load_ptr(trueObject);
        }
    }
    return load_ptr(falseObject);
}

void _String::PrimSubstringFrom_to_(Interpreter*, VMFrame* frame) {
    vm_oop_t end = frame->Pop();
    vm_oop_t start = frame->Pop();

    VMString* self = static_cast<VMString*>(frame->Pop());
    StdString str = self->GetStdString();

    int64_t s = INT_VAL(start) - 1;
    int64_t e = INT_VAL(end) - 1;

    StdString result = str.substr(s, e - s + 1);

    frame->Push(GetUniverse()->NewString(result));
}

static vm_oop_t strIsWhiteSpace(vm_oop_t rcvr) {
    VMString* self = static_cast<VMString*>(rcvr);

    size_t len = self->GetStringLength();

    const char* string = self->GetRawChars();

    for (size_t i = 0; i < len; i++) {
        if (!isspace(string[i])) {
            return load_ptr(falseObject);
        }
    }

    if (len > 0) {
        return load_ptr(trueObject);
    } else {
        return load_ptr(falseObject);
    }
}

static vm_oop_t strIsLetters(vm_oop_t rcvr) {
    VMString* self = static_cast<VMString*>(rcvr);

    size_t len = self->GetStringLength();

    const char* string = self->GetRawChars();

    for (size_t i = 0; i < len; i++) {
        if (!isalpha(string[i])) {
            return load_ptr(falseObject);
        }
    }

    if (len > 0) {
        return load_ptr(trueObject);
    } else {
        return load_ptr(falseObject);
    }
}

static vm_oop_t strIsDigits(vm_oop_t rcvr) {
    VMString* self = static_cast<VMString*>(rcvr);

    size_t len = self->GetStringLength();

    const char* string = self->GetRawChars();

    for (size_t i = 0; i < len; i++) {
        if (!isdigit(string[i])) {
            return load_ptr(falseObject);
        }
    }

    if (len > 0) {
        return load_ptr(trueObject);
    } else {
        return load_ptr(falseObject);
    }
}

_String::_String() : PrimitiveContainer() {
    Add("concatenate:", &strConcatenate_, false);
    Add("asSymbol", &strAsSymbol, false);
    Add("hashcode", &strHashcode, false);
    Add("length", &strLength, false);
    Add("=", &strEqual, false);
    SetPrimitive(
        "primSubstringFrom:to:",
        new Routine<_String>(this, &_String::PrimSubstringFrom_to_, false));
    Add("isWhiteSpace", &strIsWhiteSpace, false);
    Add("isLetters", &strIsLetters, false);
    Add("isDigits", &strIsDigits, false);
}
