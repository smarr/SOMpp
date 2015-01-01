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
#include <vmobjects/VMBigInteger.h>
#include <vm/Universe.h>

#include "Integer.h"
#include "../primitivesCore/Routine.h"

/*
 * This macro performs a coercion check to BigInteger and Double. Depending on
 * the right-hand operand, an Integer operation will have to be resent as a
 * BigInteger or Double operation (those types impose themselves on the result
 * of an Integer operation).
 */
#define CHECK_COERCION(obj,receiver,op) { \
  pVMClass cl = CLASS_OF(obj);\
  if (cl== bigIntegerClass) { \
    resendAsBigInteger((op), (receiver), static_cast<VMBigInteger*>(obj)); \
    return; \
  } else if(cl== doubleClass) { \
    resendAsDouble((op), (receiver), static_cast<VMDouble*>(obj)); \
    return; \
  } \
}

_Integer::_Integer() : PrimitiveContainer() {
    srand((unsigned) time(nullptr));
    SetPrimitive("plus",               new Routine<_Integer>(this, &_Integer::Plus));
    SetPrimitive("minus",              new Routine<_Integer>(this, &_Integer::Minus));
    SetPrimitive("star",               new Routine<_Integer>(this, &_Integer::Star));
    SetPrimitive("bitAnd_",            new Routine<_Integer>(this, &_Integer::BitwiseAnd));
    SetPrimitive("bitXor_",            new Routine<_Integer>(this, &_Integer::BitwiseXor));
    SetPrimitive("lowerthanlowerthan", new Routine<_Integer>(this, &_Integer::LeftShift));
    SetPrimitive("slash",              new Routine<_Integer>(this, &_Integer::Slash));
    SetPrimitive("slashslash",         new Routine<_Integer>(this, &_Integer::Slashslash));
    SetPrimitive("percent",            new Routine<_Integer>(this, &_Integer::Percent));
    SetPrimitive("and",                new Routine<_Integer>(this, &_Integer::And));
    SetPrimitive("equal",              new Routine<_Integer>(this, &_Integer::Equal));
    SetPrimitive("lowerthan",          new Routine<_Integer>(this, &_Integer::Lowerthan));
    SetPrimitive("asString",           new Routine<_Integer>(this, &_Integer::AsString));
    SetPrimitive("sqrt",               new Routine<_Integer>(this, &_Integer::Sqrt));
    SetPrimitive("atRandom",           new Routine<_Integer>(this, &_Integer::AtRandom));
    SetPrimitive("fromString_",        new Routine<_Integer>(this, &_Integer::FromString));
}

//
// private functions for Integer
//

void _Integer::pushResult(pVMObject /*object*/, pVMFrame frame,
                          int64_t result) {
    int32_t i32min = INT32_MIN;
    // Check with integer bounds and push:
    if(result > INT32_MAX || result < i32min)
        frame->Push(GetUniverse()->NewBigInteger(result));
    else
        frame->Push(NEW_INT((int32_t)result));
}

void _Integer::resendAsBigInteger(const char* op, oop_t left, VMBigInteger* right) {
    // Construct left value as BigInteger:
    VMBigInteger* leftBigInteger = GetUniverse()->NewBigInteger((int64_t)INT_VAL(left));

    // Resend message:
    oop_t operands[] = {right};

    leftBigInteger->Send(op, operands, 1);
    // no reference
}

void _Integer::resendAsDouble(const char* op, oop_t left, VMDouble* right) {
    VMDouble* leftDouble = GetUniverse()->NewDouble((double)INT_VAL(left));
    oop_t operands[] = {right};

    leftDouble->Send(op, operands, 1);
}

//
// arithmetic operations
//

void _Integer::Plus(pVMObject object, pVMFrame frame) {
    oop_t rightObj = frame->Pop();
    oop_t leftObj  = frame->Pop();

    CHECK_COERCION(rightObj, leftObj, "+");

    int64_t result = (int64_t)INT_VAL(leftObj) + (int64_t)INT_VAL(rightObj);
    pushResult(object, frame, result);
}

void _Integer::BitwiseAnd(pVMObject object, pVMFrame frame) {
    oop_t rightObj = frame->Pop();
    oop_t leftObj  = frame->Pop();

    int64_t result = (int64_t)INT_VAL(leftObj) & (int64_t)INT_VAL(rightObj);
    pushResult(object, frame, result);
}

void _Integer::BitwiseXor(pVMObject object, pVMFrame frame) {
    oop_t rightObj = frame->Pop();
    oop_t leftObj  = frame->Pop();
    
    int64_t result = (int64_t)INT_VAL(leftObj) ^ (int64_t)INT_VAL(rightObj);
    pushResult(object, frame, result);
}


void _Integer::LeftShift(pVMObject object, pVMFrame frame) {
    oop_t rightObj = frame->Pop();
    oop_t leftObj  = frame->Pop();
    
    int64_t result = (int64_t)INT_VAL(leftObj) << (int64_t)INT_VAL(rightObj);
    pushResult(object, frame, result);
}

void _Integer::Minus(pVMObject object, pVMFrame frame) {
    oop_t rightObj = frame->Pop();
    oop_t leftObj  = frame->Pop();
    
    CHECK_COERCION(rightObj, leftObj, "-");

    int64_t result = (int64_t)INT_VAL(leftObj) - (int64_t)INT_VAL(rightObj);
    pushResult(object, frame, result);
}

void _Integer::Star(pVMObject object, pVMFrame frame) {
    oop_t rightObj = frame->Pop();
    oop_t leftObj  = frame->Pop();
    
    CHECK_COERCION(rightObj, leftObj, "*");

    int64_t result = (int64_t)INT_VAL(leftObj) * (int64_t)INT_VAL(rightObj);
    pushResult(object, frame, result);
}

void _Integer::Slashslash(pVMObject object, pVMFrame frame) {
    oop_t rightObj = frame->Pop();
    oop_t leftObj  = frame->Pop();
    
    CHECK_COERCION(rightObj, leftObj, "/");

    double result = (double)INT_VAL(leftObj) / (double)INT_VAL(rightObj);
    frame->Push(GetUniverse()->NewDouble(result));
}

void _Integer::Slash(pVMObject object, pVMFrame frame) {
    oop_t rightObj = frame->Pop();
    oop_t leftObj  = frame->Pop();
    
    CHECK_COERCION(rightObj, leftObj, "/");

    int64_t result = (int64_t)INT_VAL(leftObj) / (int64_t)INT_VAL(rightObj);
    pushResult(object, frame, result);
}

void _Integer::Percent(pVMObject object, pVMFrame frame) {
    oop_t rightObj = frame->Pop();
    oop_t leftObj  = frame->Pop();

    CHECK_COERCION(rightObj, leftObj, "%");

    int64_t l = (int64_t)INT_VAL(leftObj);
    int64_t r = (int64_t)INT_VAL(rightObj);

    int64_t result = l % r;

    if (l > 0 && r < 0) {
        result += r;
    }

    pushResult(object, frame, result);
}

void _Integer::And(pVMObject object, pVMFrame frame) {
    oop_t rightObj = frame->Pop();
    oop_t leftObj  = frame->Pop();

    CHECK_COERCION(rightObj, leftObj, "&");

    int64_t result = (int64_t)INT_VAL(leftObj) & (int64_t)INT_VAL(rightObj);
    pushResult(object, frame, result);
}

void _Integer::Equal(pVMObject object, pVMFrame frame) {
    oop_t rightObj = frame->Pop();
    oop_t leftObj  = frame->Pop();

    CHECK_COERCION(rightObj, leftObj, "=");

    if (IS_TAGGED(rightObj) || CLASS_OF(rightObj) == integerClass) {
        if (INT_VAL(leftObj) == INT_VAL(rightObj))
            frame->Push(trueObject);
        else
            frame->Push(falseObject);
    } else if (CLASS_OF(rightObj) == doubleClass) {
        assert(false);
    } else {
        frame->Push(falseObject);
    }
}

void _Integer::Lowerthan(pVMObject object, pVMFrame frame) {
    oop_t rightObj = frame->Pop();
    oop_t leftObj  = frame->Pop();

    CHECK_COERCION(rightObj, leftObj, "<");

    if (INT_VAL(leftObj) < INT_VAL(rightObj))
        frame->Push(trueObject);
    else
        frame->Push(falseObject);
}

void _Integer::AsString(pVMObject /*object*/, pVMFrame frame) {
    oop_t self = frame->Pop();
    long integer = INT_VAL(self);
    ostringstream Str;
    Str << integer;
    frame->Push(GetUniverse()->NewString( Str.str()));
}

void _Integer::Sqrt(pVMObject object, pVMFrame frame) {
    oop_t self = frame->Pop();
    double result = sqrt((double)INT_VAL(self));

    if (result == rint(result))
        pushResult(object, frame, result);
    else
        frame->Push(GetUniverse()->NewDouble(result));
}

void _Integer::AtRandom(pVMObject /*object*/, pVMFrame frame) {
    oop_t self = frame->Pop();
    int32_t result = (INT_VAL(self) * rand())%INT32_MAX;
    frame->Push(NEW_INT(result));
}

void _Integer::FromString(pVMObject, pVMFrame frame) {
    pVMString self = (pVMString) frame->Pop();
    frame->Pop();

    int32_t integer = atoi(self->GetChars());
    oop_t new_int = NEW_INT(integer);
    frame->Push(new_int);
}

