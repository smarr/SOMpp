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
    if(dynamic_cast<pVMBigInteger>(obj) != NULL) { \
        resendAsBigInteger( \
            object, (op), (receiver), (pVMBigInteger)(obj)); \
        return; \
    } else if(dynamic_cast<pVMDouble>(obj) != NULL) { \
        resendAsDouble( \
            object, (op), (receiver), (pVMDouble)(obj)); \
        return; \
    } \
}

_Integer::_Integer( ) : PrimitiveContainer() {
    srand((unsigned) time(NULL)) ;
    this->SetPrimitive("plus", new 
        Routine<_Integer>(this, &_Integer::Plus));

    this->SetPrimitive("minus", new 
        Routine<_Integer>(this, &_Integer::Minus));

    this->SetPrimitive("star", new 
        Routine<_Integer>(this, &_Integer::Star));

    this->SetPrimitive("slash", new 
        Routine<_Integer>(this, &_Integer::Slash));

    this->SetPrimitive("slashslash", new 
        Routine<_Integer>(this, &_Integer::Slashslash));

    this->SetPrimitive("percent", new 
        Routine<_Integer>(this, &_Integer::Percent));

    this->SetPrimitive("and", new 
        Routine<_Integer>(this, &_Integer::And));
    this->SetPrimitive("equal", new 
        Routine<_Integer>(this, &_Integer::Equal));

    this->SetPrimitive("lowerthan", 
        new Routine<_Integer>(this, &_Integer::Lowerthan));
    this->SetPrimitive("asString", 
        new Routine<_Integer>(this, &_Integer::AsString));

    this->SetPrimitive("sqrt", 
        new Routine<_Integer>(this, &_Integer::Sqrt));

    this->SetPrimitive("atRandom", 
        new Routine<_Integer>(this, &_Integer::AtRandom));
}

//
// private functions for Integer
//


void _Integer::pushResult(pVMObject /*object*/, pVMFrame frame, 
                              int64_t result) {
    int32_t i32min = INT32_MIN;
    // Check with integer bounds and push:
    if(result > INT32_MAX || result < i32min)
        frame->Push((pVMObject)_UNIVERSE->NewBigInteger(result));
    else
        frame->Push((pVMObject)_UNIVERSE->NewInteger((int32_t)result));
}


void _Integer::resendAsBigInteger(pVMObject /*object*/, 
                                  const char* op,
                                  pVMInteger left, pVMBigInteger right) {
    // Construct left value as BigInteger:
    pVMBigInteger leftBigInteger = 
        _UNIVERSE->NewBigInteger((int64_t)left->GetEmbeddedInteger());
    
    // Resend message:
    pVMObject operands[] = { (pVMObject)right };
    
    leftBigInteger->Send(op, operands, 1);
    // no reference
}


void _Integer::resendAsDouble(pVMObject /*object*/, const char* op,
    pVMInteger left, pVMDouble right
) {
    pVMDouble leftDouble =
        _UNIVERSE->NewDouble((double)left->GetEmbeddedInteger());
    pVMObject operands[] = { (pVMObject)right };
    
    leftDouble->Send(op, operands, 1);
}



//
// arithmetic operations
//


void  _Integer::Plus(pVMObject object, pVMFrame frame) {
    pVMObject rightObj = frame->Pop();
    pVMInteger left = (pVMInteger)frame->Pop();
    
    CHECK_COERCION(rightObj, left, "+");

    // Do operation:
    pVMInteger right = (pVMInteger)rightObj;
    
    int64_t result = (int64_t)left->GetEmbeddedInteger() + 
        (int64_t)right->GetEmbeddedInteger();
    pushResult(object, frame, result);
}


void  _Integer::Minus(pVMObject object, pVMFrame frame) {
    pVMObject rightObj = frame->Pop();
    pVMInteger left = (pVMInteger)frame->Pop();
    
    CHECK_COERCION(rightObj, left, "-");

    // Do operation:
    pVMInteger right = (pVMInteger)rightObj;
    
    int64_t result = (int64_t)left->GetEmbeddedInteger() - 
        (int64_t)right->GetEmbeddedInteger();
    pushResult(object, frame, result);
}


void  _Integer::Star(pVMObject object, pVMFrame frame) {
    pVMObject rightObj = frame->Pop();
    pVMInteger left = (pVMInteger)frame->Pop();
    
    CHECK_COERCION(rightObj, left, "*");

    // Do operation:
    pVMInteger right = (pVMInteger)rightObj;
    
    int64_t result = (int64_t)left->GetEmbeddedInteger() * 
        (int64_t)right->GetEmbeddedInteger();
    pushResult(object, frame, result); 
}


void  _Integer::Slashslash(pVMObject object, pVMFrame frame) {
    pVMObject rightObj = frame->Pop();
    pVMInteger left = (pVMInteger)frame->Pop();
    
    CHECK_COERCION(rightObj, left, "/");

    // Do operation:
    pVMInteger right = (pVMInteger)rightObj;
    
    double result = (double)left->GetEmbeddedInteger() /
        (double)right->GetEmbeddedInteger();
    frame->Push(_UNIVERSE->NewDouble(result));
}


void  _Integer::Slash(pVMObject object, pVMFrame frame) {
    pVMObject rightObj = frame->Pop();
    pVMInteger left = (pVMInteger)frame->Pop();
    
    CHECK_COERCION(rightObj, left, "/");

    // Do operation:
    pVMInteger right = (pVMInteger)rightObj;
    
    int64_t result = (int64_t)left->GetEmbeddedInteger() / 
        (int64_t)right->GetEmbeddedInteger();
    pushResult(object, frame, result); 
}


void  _Integer::Percent(pVMObject object, pVMFrame frame) {
    pVMObject rightObj = frame->Pop();
    pVMInteger left = (pVMInteger)frame->Pop();
    
    CHECK_COERCION(rightObj, left, "%");

    // Do operation:
    pVMInteger right = (pVMInteger)rightObj;

    int64_t result = (int64_t)left->GetEmbeddedInteger() %
        (int64_t)right->GetEmbeddedInteger();
    pushResult(object, frame, result); 
}


void  _Integer::And(pVMObject object, pVMFrame frame) {
    pVMObject rightObj = frame->Pop();
    pVMInteger left = (pVMInteger)frame->Pop();
    
    CHECK_COERCION(rightObj, left, "&");

    // Do operation:
    pVMInteger right = (pVMInteger)rightObj;
    
    int64_t result = (int64_t)left->GetEmbeddedInteger() & 
        (int64_t)right->GetEmbeddedInteger();
    pushResult(object, frame, result); 
}   


void  _Integer::Equal(pVMObject object, pVMFrame frame) {
    pVMObject rightObj = frame->Pop();
    pVMInteger left = (pVMInteger)frame->Pop();
    
    CHECK_COERCION(rightObj, left, "=");

    if(dynamic_cast<pVMInteger>(rightObj) != NULL) {
        // Second operand was Integer:
        pVMInteger right = (pVMInteger)rightObj;
        
        if(left->GetEmbeddedInteger()
            == right->GetEmbeddedInteger())
            frame->Push(trueObject);
        else
            frame->Push(falseObject);
    } else if(dynamic_cast<pVMDouble>(rightObj) != NULL) {
        // Second operand was Double:
        pVMDouble right = (pVMDouble)rightObj;
        
        if((double)left->GetEmbeddedInteger()
            == right->GetEmbeddedDouble())
            frame->Push(trueObject);
        else
            frame->Push(falseObject);
    }
    else
        frame->Push(falseObject);
}


void  _Integer::Lowerthan(pVMObject object, pVMFrame frame) {
    pVMObject rightObj = frame->Pop();
    pVMInteger left = (pVMInteger)frame->Pop();
    
    CHECK_COERCION(rightObj, left, "<");

    pVMInteger right = (pVMInteger)rightObj;
    
    if(left->GetEmbeddedInteger() < right->GetEmbeddedInteger())
        frame->Push(trueObject);
    else
        frame->Push(falseObject);
}


void  _Integer::AsString(pVMObject /*object*/, pVMFrame frame) {
    pVMInteger self = (pVMInteger)frame->Pop();
    
    int32_t integer = self->GetEmbeddedInteger();
    ostringstream Str;
    Str << integer;
    frame->Push( (pVMObject)_UNIVERSE->NewString( Str.str() ) );   
}


void  _Integer::Sqrt(pVMObject /*object*/, pVMFrame frame) {
    pVMInteger self = (pVMInteger)frame->Pop();
    double result = sqrt((double)self->GetEmbeddedInteger());
    frame->Push((pVMObject)_UNIVERSE->NewDouble(result));
}


void  _Integer::AtRandom(pVMObject /*object*/, pVMFrame frame) {
    pVMInteger self = (pVMInteger)frame->Pop();
    int32_t result = (self->GetEmbeddedInteger() * rand())%INT32_MAX;
    frame->Push((pVMObject) _UNIVERSE->NewInteger(result));
}



