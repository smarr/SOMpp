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

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <limits.h>
#include <sstream>

#include <vmobjects/VMObject.h>
#include <vmobjects/VMFrame.h>
#include <vmobjects/VMDouble.h>
#include <vmobjects/VMInteger.h>
#include <vmobjects/VMString.h>
#include <vm/Universe.h>

#include "Integer.h"
#include "../primitivesCore/Routine.h"

/*
 * This macro performs a coercion check to Double. Depending on
 * the right-hand operand, an Integer operation will have to be resent as a
 * Double operation (this type imposes itselves on the result
 * of an Integer operation).
 */
#define CHECK_COERCION(obj,receiver,op) { \
  VMClass* cl = CLASS_OF(obj);\
  if(cl == load_ptr(doubleClass)) { \
    resendAsDouble(interp, (op), (receiver), static_cast<VMDouble*>(obj)); \
    return; \
  } \
}

_Integer::_Integer() : PrimitiveContainer() {
    srand((unsigned) time(nullptr));
    SetPrimitive("plus",               new Routine<_Integer>(this, &_Integer::Plus,       false));
    SetPrimitive("minus",              new Routine<_Integer>(this, &_Integer::Minus,      false));
    SetPrimitive("star",               new Routine<_Integer>(this, &_Integer::Star,       false));
    SetPrimitive("rem_",               new Routine<_Integer>(this, &_Integer::Rem,        false));
    SetPrimitive("bitAnd_",            new Routine<_Integer>(this, &_Integer::BitwiseAnd, false));
    SetPrimitive("bitXor_",            new Routine<_Integer>(this, &_Integer::BitwiseXor, false));
    SetPrimitive("lowerthanlowerthan", new Routine<_Integer>(this, &_Integer::LeftShift,  false));
    SetPrimitive("greaterthangreaterthangreaterthan", new Routine<_Integer>(this, &_Integer::UnsignedRightShift, false));
    SetPrimitive("slash",              new Routine<_Integer>(this, &_Integer::Slash,      false));
    SetPrimitive("slashslash",         new Routine<_Integer>(this, &_Integer::Slashslash, false));
    SetPrimitive("percent",            new Routine<_Integer>(this, &_Integer::Percent,    false));
    SetPrimitive("and",                new Routine<_Integer>(this, &_Integer::And,        false));
    SetPrimitive("equal",              new Routine<_Integer>(this, &_Integer::Equal,      false));
    SetPrimitive("equalequal",         new Routine<_Integer>(this, &_Integer::EqualEqual, false));
    SetPrimitive("lowerthan",          new Routine<_Integer>(this, &_Integer::Lowerthan,  false));
    SetPrimitive("asString",           new Routine<_Integer>(this, &_Integer::AsString,   false));
    SetPrimitive("as32BitSignedValue", new Routine<_Integer>(this, &_Integer::As32BitSigned, false));
    SetPrimitive("as32BitUnsignedValue", new Routine<_Integer>(this, &_Integer::As32BitUnsigned, false));
    SetPrimitive("sqrt",               new Routine<_Integer>(this, &_Integer::Sqrt,       false));
    SetPrimitive("atRandom",           new Routine<_Integer>(this, &_Integer::AtRandom,   false));
    SetPrimitive("fromString_",        new Routine<_Integer>(this, &_Integer::FromString, true));
}

//
// private functions for Integer
//

void _Integer::resendAsDouble(Interpreter* interp, const char* op, vm_oop_t left, VMDouble* right) {
    VMDouble* leftDouble = GetUniverse()->NewDouble((double)INT_VAL(left));
    vm_oop_t operands[] = {right};

    leftDouble->Send(interp, op, operands, 1);
}

//
// arithmetic operations
//

void _Integer::Plus(Interpreter* interp, VMFrame* frame) {
    vm_oop_t rightObj = frame->Pop();
    vm_oop_t leftObj  = frame->Pop();

    CHECK_COERCION(rightObj, leftObj, "+");

    int64_t result = (int64_t)INT_VAL(leftObj) + (int64_t)INT_VAL(rightObj);
    frame->Push(NEW_INT(result));
}

void _Integer::BitwiseAnd(Interpreter* interp, VMFrame* frame) {
    vm_oop_t rightObj = frame->Pop();
    vm_oop_t leftObj  = frame->Pop();

    int64_t result = (int64_t)INT_VAL(leftObj) & (int64_t)INT_VAL(rightObj);
    frame->Push(NEW_INT(result));
}

void _Integer::BitwiseXor(Interpreter* interp, VMFrame* frame) {
    vm_oop_t rightObj = frame->Pop();
    vm_oop_t leftObj  = frame->Pop();
    
    int64_t result = (int64_t)INT_VAL(leftObj) ^ (int64_t)INT_VAL(rightObj);
    frame->Push(NEW_INT(result));
}


void _Integer::LeftShift(Interpreter* interp, VMFrame* frame) {
    vm_oop_t rightObj = frame->Pop();
    vm_oop_t leftObj  = frame->Pop();
    
    int64_t result = (int64_t)INT_VAL(leftObj) << (int64_t)INT_VAL(rightObj);
    frame->Push(NEW_INT(result));
}

void _Integer::UnsignedRightShift(Interpreter* interp, VMFrame* frame) {
    vm_oop_t rightObj = frame->Pop();
    vm_oop_t leftObj  = frame->Pop();
    
    int64_t result = (int64_t)INT_VAL(leftObj) >> (int64_t)INT_VAL(rightObj);
    frame->Push(NEW_INT(result));
}


void _Integer::Minus(Interpreter* interp, VMFrame* frame) {
    vm_oop_t rightObj = frame->Pop();
    vm_oop_t leftObj  = frame->Pop();
    
    CHECK_COERCION(rightObj, leftObj, "-");

    int64_t result = (int64_t)INT_VAL(leftObj) - (int64_t)INT_VAL(rightObj);
    frame->Push(NEW_INT(result));
}

void _Integer::Star(Interpreter* interp, VMFrame* frame) {
    vm_oop_t rightObj = frame->Pop();
    vm_oop_t leftObj  = frame->Pop();
    
    CHECK_COERCION(rightObj, leftObj, "*");

    int64_t result = (int64_t)INT_VAL(leftObj) * (int64_t)INT_VAL(rightObj);
    frame->Push(NEW_INT(result));
}

void _Integer::Slashslash(Interpreter* interp, VMFrame* frame) {
    vm_oop_t rightObj = frame->Pop();
    vm_oop_t leftObj  = frame->Pop();
    
    CHECK_COERCION(rightObj, leftObj, "//");

    double result = (double)INT_VAL(leftObj) / (double)INT_VAL(rightObj);
    frame->Push(GetUniverse()->NewDouble(result));
}

void _Integer::Slash(Interpreter* interp, VMFrame* frame) {
    vm_oop_t rightObj = frame->Pop();
    vm_oop_t leftObj  = frame->Pop();
    
    CHECK_COERCION(rightObj, leftObj, "/");

    int64_t result = (int64_t)INT_VAL(leftObj) / (int64_t)INT_VAL(rightObj);
    frame->Push(NEW_INT(result));
}

void _Integer::Percent(Interpreter* interp, VMFrame* frame) {
    vm_oop_t rightObj = frame->Pop();
    vm_oop_t leftObj  = frame->Pop();

    CHECK_COERCION(rightObj, leftObj, "%");

    int64_t l = (int64_t)INT_VAL(leftObj);
    int64_t r = (int64_t)INT_VAL(rightObj);

    int64_t result = l % r;
    
    if ((result != 0) && ((result < 0) != (r < 0))) {
        result += r;
    }

    frame->Push(NEW_INT(result));
}

void _Integer::Rem(Interpreter* interp, VMFrame* frame) {
    vm_oop_t rightObj = frame->Pop();
    vm_oop_t leftObj  = frame->Pop();
    
    CHECK_COERCION(rightObj, leftObj, "%");
    
    int64_t l = (int64_t)INT_VAL(leftObj);
    int64_t r = (int64_t)INT_VAL(rightObj);
    
    int64_t result = l - (l / r) * r;
    
    frame->Push(NEW_INT(result));
}

void _Integer::And(Interpreter* interp, VMFrame* frame) {
    vm_oop_t rightObj = frame->Pop();
    vm_oop_t leftObj  = frame->Pop();

    CHECK_COERCION(rightObj, leftObj, "&");

    int64_t result = (int64_t)INT_VAL(leftObj) & (int64_t)INT_VAL(rightObj);
    frame->Push(NEW_INT(result));
}

void _Integer::Equal(Interpreter* interp, VMFrame* frame) {
    vm_oop_t rightObj = frame->Pop();
    vm_oop_t leftObj  = frame->Pop();

    CHECK_COERCION(rightObj, leftObj, "=");

    if (IS_TAGGED(rightObj) || CLASS_OF(rightObj) == load_ptr(integerClass)) {
        if (INT_VAL(leftObj) == INT_VAL(rightObj))
            frame->Push(load_ptr(trueObject));
        else
            frame->Push(load_ptr(falseObject));
    } else if (CLASS_OF(rightObj) == load_ptr(doubleClass)) {
        assert(false);
    } else {
        frame->Push(load_ptr(falseObject));
    }
}

void _Integer::EqualEqual(Interpreter* interp, VMFrame* frame) {
    vm_oop_t rightObj = frame->Pop();
    vm_oop_t leftObj  = frame->Pop();
    
    if (IS_TAGGED(rightObj) || CLASS_OF(rightObj) == load_ptr(integerClass)) {
        if (INT_VAL(leftObj) == INT_VAL(rightObj))
            frame->Push(load_ptr(trueObject));
        else
            frame->Push(load_ptr(falseObject));
    } else {
        frame->Push(load_ptr(falseObject));
    }
}

void _Integer::Lowerthan(Interpreter* interp, VMFrame* frame) {
    vm_oop_t rightObj = frame->Pop();
    vm_oop_t leftObj  = frame->Pop();

    CHECK_COERCION(rightObj, leftObj, "<");

    if (INT_VAL(leftObj) < INT_VAL(rightObj))
        frame->Push(load_ptr(trueObject));
    else
        frame->Push(load_ptr(falseObject));
}

void _Integer::AsString(Interpreter*, VMFrame* frame) {
    vm_oop_t self = frame->Pop();
    long integer = INT_VAL(self);
    ostringstream Str;
    Str << integer;
    frame->Push(GetUniverse()->NewString( Str.str()));
}

void _Integer::As32BitSigned(Interpreter*, VMFrame* frame) {
    vm_oop_t self = frame->Pop();
    int64_t integer = INT_VAL(self);
    
    frame->Push(NEW_INT((int64_t)(int32_t) integer));
}

void _Integer::As32BitUnsigned(Interpreter*, VMFrame* frame) {
    vm_oop_t self = frame->Pop();
    int64_t integer = INT_VAL(self);
    
    frame->Push(NEW_INT((int64_t)(uint32_t) integer));
}

void _Integer::Sqrt(Interpreter*, VMFrame* frame) {
    vm_oop_t self = frame->Pop();
    double result = sqrt((double)INT_VAL(self));

    if (result == rint(result))
        frame->Push(NEW_INT((int64_t) result));
    else
        frame->Push(GetUniverse()->NewDouble(result));
}

void _Integer::AtRandom(Interpreter*, VMFrame* frame) {
    vm_oop_t self = frame->Pop();
    int64_t result = INT_VAL(self) * rand();
    frame->Push(NEW_INT(result));
}

void _Integer::FromString(Interpreter*, VMFrame* frame) {
    VMString* self = (VMString*) frame->Pop();
    frame->Pop();

    int64_t integer = atol(self->GetChars());
    vm_oop_t new_int = NEW_INT(integer);
    frame->Push(new_int);
}

