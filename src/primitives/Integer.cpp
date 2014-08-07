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
#ifdef USE_TAGGING
#define CHECK_COERCION(obj,receiver,op) { \
  pVMClass cl = (IS_TAGGED(obj) ? integerClass : AS_POINTER(obj)->GetClass());\
  if (cl== bigIntegerClass) { \
    resendAsBigInteger( \
                       object, (op), (receiver), static_cast<pVMBigInteger>(obj)); \
    return; \
  } else if(cl== doubleClass) { \
    resendAsDouble( \
                   object, (op), (receiver), static_cast<pVMDouble>(obj)); \
    return; \
  } \
}
#else
#define CHECK_COERCION(obj,receiver,op) {\
    pVMClass cl = ((AbstractVMObject*)obj)->GetClass(); \
    PG_HEAP(ReadBarrier((void**)&bigIntegerClass));\
    PG_HEAP(ReadBarrier((void**)&doubleClass));\
    if (UNTAG_REFERENCE(cl) == UNTAG_REFERENCE(bigIntegerClass)) { \
        resendAsBigInteger( \
            object, (op), (receiver), static_cast<pVMBigInteger>(obj)); \
        return; \
    } else if (UNTAG_REFERENCE(cl) == UNTAG_REFERENCE(doubleClass)) { \
        resendAsDouble( \
            object, (op), (receiver), static_cast<pVMDouble>(obj)); \
        return; \
    } \
}
#endif

_Integer::_Integer() :
        PrimitiveContainer() {
    srand((unsigned) time(NULL));
    this->SetPrimitive("plus", new Routine<_Integer>(this, &_Integer::Plus));

    this->SetPrimitive("minus", new Routine<_Integer>(this, &_Integer::Minus));

    this->SetPrimitive("star", new Routine<_Integer>(this, &_Integer::Star));

    this->SetPrimitive("bitAnd_",
            new Routine<_Integer>(this, &_Integer::BitwiseAnd));

    this->SetPrimitive("slash", new Routine<_Integer>(this, &_Integer::Slash));

    this->SetPrimitive("slashslash",
            new Routine<_Integer>(this, &_Integer::Slashslash));

    this->SetPrimitive("percent",
            new Routine<_Integer>(this, &_Integer::Percent));

    this->SetPrimitive("and", new Routine<_Integer>(this, &_Integer::And));
    this->SetPrimitive("equal", new Routine<_Integer>(this, &_Integer::Equal));

    this->SetPrimitive("lowerthan",
            new Routine<_Integer>(this, &_Integer::Lowerthan));
    this->SetPrimitive("asString",
            new Routine<_Integer>(this, &_Integer::AsString));

    this->SetPrimitive("sqrt", new Routine<_Integer>(this, &_Integer::Sqrt));

    this->SetPrimitive("atRandom",
            new Routine<_Integer>(this, &_Integer::AtRandom));

    this->SetPrimitive("fromString_",
            new Routine<_Integer>(this, &_Integer::FromString));
}

//
// private functions for Integer
//

void _Integer::pushResult(pVMObject /*object*/, pVMFrame frame,
int64_t result) {
    int32_t i32min = INT32_MIN;
    // Check with integer bounds and push:
    if(result > INT32_MAX || result < i32min)
    frame->Push(_UNIVERSE->NewBigInteger(result));
    else
#ifdef USE_TAGGING
    frame->Push(TAG_INTEGER((int32_t)result));
#else
    frame->Push(_UNIVERSE->NewInteger((int32_t)result));
#endif
}

void _Integer::resendAsBigInteger(pVMObject /*object*/, const char* op,
pVMInteger left, pVMBigInteger right) {
    // Construct left value as BigInteger:
    pVMBigInteger leftBigInteger =
#ifdef USE_TAGGING
    _UNIVERSE->NewBigInteger((int64_t)UNTAG_INTEGER(left));
#else
    _UNIVERSE->NewBigInteger((int64_t)left->GetEmbeddedInteger());
#endif

    // Resend message:
    pVMObject operands[] = {right};

    leftBigInteger->Send(op, operands, 1);
    // no reference
}

void _Integer::resendAsDouble(pVMObject /*object*/, const char* op,
pVMInteger left, pVMDouble right
) {
    pVMDouble leftDouble =
#ifdef USE_TAGGING
    _UNIVERSE->NewDouble((double)UNTAG_INTEGER(left));
#else
    _UNIVERSE->NewDouble((double)left->GetEmbeddedInteger());
#endif
    pVMObject operands[] = {right};

    leftDouble->Send(op, operands, 1);
}

//
// arithmetic operations
//

void _Integer::Plus(pVMObject object, pVMFrame frame) {
    pVMObject rightObj = frame->Pop();
    pVMInteger left = static_cast<pVMInteger>(frame->Pop());

    CHECK_COERCION(rightObj, left, "+");

    // Do operation:
    pVMInteger right = static_cast<pVMInteger>(rightObj);

#ifdef USE_TAGGING
    int64_t result = (int64_t)UNTAG_INTEGER(left) +
    (int64_t)UNTAG_INTEGER(right);
#else
    int64_t result = (int64_t)left->GetEmbeddedInteger() +
    (int64_t)right->GetEmbeddedInteger();
#endif
    pushResult(object, frame, result);
}

void _Integer::BitwiseAnd(pVMObject object, pVMFrame frame) {
    pVMInteger right = static_cast<pVMInteger>(frame->Pop());
    pVMInteger left = static_cast<pVMInteger>(frame->Pop());

#ifdef USE_TAGGING
    int64_t result = (int64_t)UNTAG_INTEGER(left) & (int64_t)UNTAG_INTEGER(right);
#else
    int64_t result = (int64_t)left->GetEmbeddedInteger() & (int64_t)right->GetEmbeddedInteger();
#endif
    pushResult(object, frame, result);
}

void _Integer::Minus(pVMObject object, pVMFrame frame) {
    pVMObject rightObj = frame->Pop();
    pVMInteger left = static_cast<pVMInteger>(frame->Pop());

    CHECK_COERCION(rightObj, left, "-");

    // Do operation:
    pVMInteger right = static_cast<pVMInteger>(rightObj);

#ifdef USE_TAGGING
    int64_t result = (int64_t)UNTAG_INTEGER(left) -
    (int64_t)UNTAG_INTEGER(right);
#else
    int64_t result = (int64_t)left->GetEmbeddedInteger() -
    (int64_t)right->GetEmbeddedInteger();
#endif
    pushResult(object, frame, result);
}

void _Integer::Star(pVMObject object, pVMFrame frame) {
    pVMObject rightObj = frame->Pop();
    pVMInteger left = static_cast<pVMInteger>(frame->Pop());

    CHECK_COERCION(rightObj, left, "*");

    // Do operation:
    pVMInteger right = static_cast<pVMInteger>(rightObj);

#ifdef USE_TAGGING
    int64_t result = (int64_t)UNTAG_INTEGER(left) *
    (int64_t)UNTAG_INTEGER(right);
#else
    int64_t result = (int64_t)left->GetEmbeddedInteger() *
    (int64_t)right->GetEmbeddedInteger();
#endif
    pushResult(object, frame, result);
}

void _Integer::Slashslash(pVMObject object, pVMFrame frame) {
    pVMObject rightObj = frame->Pop();
    pVMInteger left = static_cast<pVMInteger>(frame->Pop());

    CHECK_COERCION(rightObj, left, "/");

    // Do operation:
    pVMInteger right = static_cast<pVMInteger>(rightObj);

#ifdef USE_TAGGING
    double result = (double)UNTAG_INTEGER(left) /
    (double)UNTAG_INTEGER(right);
#else
    double result = (double)left->GetEmbeddedInteger() /
    (double)right->GetEmbeddedInteger();
#endif
    frame->Push(_UNIVERSE->NewDouble(result));
}

void _Integer::Slash(pVMObject object, pVMFrame frame) {
    pVMObject rightObj = frame->Pop();
    pVMInteger left = static_cast<pVMInteger>(frame->Pop());

    CHECK_COERCION(rightObj, left, "/");

    // Do operation:
    pVMInteger right = static_cast<pVMInteger>(rightObj);

#ifdef USE_TAGGING
    int64_t result = (int64_t)UNTAG_INTEGER(left) /
    (int64_t)UNTAG_INTEGER(right);
#else
    int64_t result = (int64_t)left->GetEmbeddedInteger() /
    (int64_t)right->GetEmbeddedInteger();
#endif
    pushResult(object, frame, result);
}

void _Integer::Percent(pVMObject object, pVMFrame frame) {
    pVMObject rightObj = frame->Pop();
    pVMInteger left = static_cast<pVMInteger>(frame->Pop());

    CHECK_COERCION(rightObj, left, "%");

    // Do operation:
    pVMInteger right = static_cast<pVMInteger>(rightObj);

#ifdef USE_TAGGING
    int64_t l = (int64_t)UNTAG_INTEGER(left);
    int64_t r = (int64_t)UNTAG_INTEGER(right);
#else
    int64_t l = (int64_t)left->GetEmbeddedInteger();
    int64_t r = (int64_t)right->GetEmbeddedInteger();
#endif

    int64_t result = l % r;

    if (l > 0 && r < 0) {
        result += r;
    }

    pushResult(object, frame, result);
}

void _Integer::And(pVMObject object, pVMFrame frame) {
    pVMObject rightObj = frame->Pop();
    pVMInteger left = static_cast<pVMInteger>(frame->Pop());

    CHECK_COERCION(rightObj, left, "&");

    // Do operation:
    pVMInteger right = static_cast<pVMInteger>(rightObj);

#ifdef USE_TAGGING
    int64_t result = (int64_t)UNTAG_INTEGER(left) &
    (int64_t)UNTAG_INTEGER(right);
#else
    int64_t result = (int64_t)left->GetEmbeddedInteger() &
    (int64_t)right->GetEmbeddedInteger();
#endif
    pushResult(object, frame, result);
}

void _Integer::Equal(pVMObject object, pVMFrame frame) {
    pVMObject rightObj = frame->Pop();
    pVMInteger left = static_cast<pVMInteger>(frame->Pop());

    CHECK_COERCION(rightObj, left, "=");

#ifdef USE_TAGGING
    if (IS_TAGGED(rightObj) || AS_POINTER(rightObj)->GetClass() == integerClass) {
        if (UNTAG_INTEGER(left) == UNTAG_INTEGER(rightObj))
        frame->Push(trueObject);
        else
        frame->Push(falseObject);
    } else if (AS_POINTER(rightObj)->GetClass() == doubleClass) {
        assert(false);
    } else
        frame->Push(falseObject);
#else
    PG_HEAP(ReadBarrier((void**)&integerClass));
    PG_HEAP(ReadBarrier((void**)&doubleClass));
    if (UNTAG_REFERENCE(rightObj->GetClass()) == UNTAG_REFERENCE(integerClass)) {
        pVMInteger right = static_cast<pVMInteger>(rightObj);
        if (left->GetEmbeddedInteger() == right->GetEmbeddedInteger()) {
            PG_HEAP(ReadBarrier((void**)&trueObject));
            frame->Push(trueObject);
        } else {
            PG_HEAP(ReadBarrier((void**)&falseObject));
            frame->Push(falseObject);
        }
    } else if (UNTAG_REFERENCE(rightObj->GetClass()) == UNTAG_REFERENCE(doubleClass)) {
        assert(false);
    }
    else {
        PG_HEAP(ReadBarrier((void**)falseObject));
        frame->Push(falseObject);
    }
#endif

}

void _Integer::Lowerthan(pVMObject object, pVMFrame frame) {
    pVMObject rightObj = frame->Pop();
    pVMInteger left = static_cast<pVMInteger>(frame->Pop());

    CHECK_COERCION(rightObj, left, "<");

    pVMInteger right = static_cast<pVMInteger>(rightObj);

#ifdef USE_TAGGING
    if(UNTAG_INTEGER(left) < UNTAG_INTEGER(right))
#else
    if(left->GetEmbeddedInteger() < right->GetEmbeddedInteger()) {
#endif
        PG_HEAP(ReadBarrier((void**)&trueObject));
        frame->Push(trueObject);
    } else {
        PG_HEAP(ReadBarrier((void**)&falseObject));
        frame->Push(falseObject);
    }
}

void _Integer::AsString(pVMObject /*object*/, pVMFrame frame) {
    pVMInteger self = static_cast<pVMInteger>(frame->Pop());

#ifdef USE_TAGGING
    long integer = UNTAG_INTEGER(self);
#else
    long integer = self->GetEmbeddedInteger();
#endif
    ostringstream Str;
    Str << integer;
    frame->Push(_UNIVERSE->NewString( Str.str()));
}

void _Integer::Sqrt(pVMObject object, pVMFrame frame) {
    pVMInteger self = static_cast<pVMInteger>(frame->Pop());
#ifdef USE_TAGGING
    double result = sqrt((double)UNTAG_INTEGER(self));
#else
    double result = sqrt((double)self->GetEmbeddedInteger());
#endif

    if (result == rint(result))
    pushResult(object, frame, result);
    else
    frame->Push(_UNIVERSE->NewDouble(result));
}

void _Integer::AtRandom(pVMObject /*object*/, pVMFrame frame) {
    pVMInteger self = static_cast<pVMInteger>(frame->Pop());
#ifdef USE_TAGGING
    int32_t result = (UNTAG_INTEGER(self) * rand())%INT32_MAX;
    frame->Push(TAG_INTEGER(result));
#else
    int32_t result = (self->GetEmbeddedInteger() * rand())%INT32_MAX;
    frame->Push(_UNIVERSE->NewInteger(result));
#endif
}

void _Integer::FromString(pVMObject, pVMFrame frame) {
    pVMString self = (pVMString) frame->Pop();
    frame->Pop();

    int32_t integer = atoi(self->GetChars());

#ifdef USE_TAGGING
        pVMInteger new_int = TAG_INTEGER(integer);
#else
        pVMInteger new_int = _UNIVERSE->NewInteger(integer);
#endif

        frame->Push(new_int);
    }

