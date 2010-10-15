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


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sstream> 

#include "../primitivesCore/Routine.h"
 
#include "BigInteger.h"

#include <vmobjects/VMObject.h>
#include <vmobjects/VMFrame.h>
#include <vmobjects/VMInteger.h>
#include <vmobjects/VMBigInteger.h>

#include <vm/Universe.h>


#define CHECK_BIGINT(object, result) { \
    /* Check second parameter type: */ \
    pVMInteger ptr;\
    if((ptr = dynamic_cast<pVMInteger>(object)) != NULL) { \
        /* Second operand was Integer*/ \
        int32_t i = ptr->GetEmbeddedInteger(); \
        (result) = _UNIVERSE->NewBigInteger((int64_t)i); \
    } else \
        (result) = (pVMBigInteger)(object); \
}

#define PUSH_INT_OR_BIGINT(result) { \
    if(result > INT32_MAX ||result < INT32_MIN) \
        frame->Push((pVMObject)_UNIVERSE->NewBigInteger((result))); \
    else \
        frame->Push((pVMObject)_UNIVERSE->NewInteger((int32_t)(result))); \
}
//^^DIFFERENT THAN CSOM! Does the CSOM version work at all????????


_BigInteger::_BigInteger( ) : PrimitiveContainer(){
    this->SetPrimitive("plus", static_cast<PrimitiveRoutine*>(
        new Routine<_BigInteger>(this, &_BigInteger::Plus)));

    this->SetPrimitive("minus", static_cast<PrimitiveRoutine*>(
        new Routine<_BigInteger>(this, &_BigInteger::Minus)));

    this->SetPrimitive("star", static_cast<PrimitiveRoutine*>(
        new Routine<_BigInteger>(this, &_BigInteger::Star)));

    this->SetPrimitive("slash", static_cast<PrimitiveRoutine*>( 
        new Routine<_BigInteger>(this, &_BigInteger::Slash)));

    this->SetPrimitive("percent", static_cast<PrimitiveRoutine*>(
        new Routine<_BigInteger>(this, &_BigInteger::Percent)));

    this->SetPrimitive("and", static_cast<PrimitiveRoutine*>(
        new Routine<_BigInteger>(this, &_BigInteger::And)));

    this->SetPrimitive("equal", static_cast<PrimitiveRoutine*>(
        new Routine<_BigInteger>(this, &_BigInteger::Equal)));

    this->SetPrimitive("lowerthan", static_cast<PrimitiveRoutine*>(
        new Routine<_BigInteger>(this, &_BigInteger::Lowerthan)));

    this->SetPrimitive("asString", static_cast<PrimitiveRoutine*>(
        new Routine<_BigInteger>(this, &_BigInteger::AsString)));

    this->SetPrimitive("sqrt", static_cast<PrimitiveRoutine*>(
        new Routine<_BigInteger>(this, &_BigInteger::Sqrt)));

}


void  _BigInteger::Plus(pVMObject /*object*/, pVMFrame frame) {
    pVMObject rightObj  = frame->Pop();
    pVMBigInteger right = NULL;
    pVMBigInteger left  = (pVMBigInteger)frame->Pop();
    
    CHECK_BIGINT(rightObj, right);
    
    // Do operation and perform conversion to Integer if required
    int64_t result = left->GetEmbeddedInteger()
                    + right->GetEmbeddedInteger();
    PUSH_INT_OR_BIGINT(result);
}


void  _BigInteger::Minus(pVMObject /*object*/, pVMFrame frame) {
    pVMObject rightObj  = frame->Pop();
    pVMBigInteger right = NULL;
    pVMBigInteger left  = (pVMBigInteger)frame->Pop();
    
    CHECK_BIGINT(rightObj, right);   
    
    // Do operation and perform conversion to Integer if required
    int64_t result =  left->GetEmbeddedInteger()
                    - right->GetEmbeddedInteger();
    PUSH_INT_OR_BIGINT(result);
}


void  _BigInteger::Star(pVMObject /*object*/, pVMFrame frame) {
   pVMObject rightObj  = frame->Pop();
    pVMBigInteger right = NULL;
    pVMBigInteger left  = (pVMBigInteger)frame->Pop();
    
    CHECK_BIGINT(rightObj, right);   
    
    // Do operation and perform conversion to Integer if required
    int64_t result =  left->GetEmbeddedInteger()
                    * right->GetEmbeddedInteger();
    PUSH_INT_OR_BIGINT(result);
}


void  _BigInteger::Slash(pVMObject /*object*/, pVMFrame frame) {
    pVMObject rightObj  = frame->Pop();
    pVMBigInteger right = NULL;
    pVMBigInteger left  = (pVMBigInteger)frame->Pop();
    
    CHECK_BIGINT(rightObj, right);   
    
    // Do operation and perform conversion to Integer if required
    int64_t result =  left->GetEmbeddedInteger()
                    / right->GetEmbeddedInteger();
    PUSH_INT_OR_BIGINT(result);
}


void  _BigInteger::Percent(pVMObject /*object*/, pVMFrame frame) {
    pVMObject rightObj  = frame->Pop();
    pVMBigInteger right = NULL;
    pVMBigInteger left  = (pVMBigInteger)frame->Pop();
    
    CHECK_BIGINT(rightObj, right);   
    
    // Do operation and perform conversion to Integer if required
    pVMBigInteger result = _UNIVERSE->NewBigInteger(  left->GetEmbeddedInteger()
                                                    % right->GetEmbeddedInteger());
    
    frame->Push((pVMObject) result);
}


void  _BigInteger::And(pVMObject /*object*/, pVMFrame frame) {
    pVMObject rightObj  = frame->Pop();
    pVMBigInteger right = NULL;
    pVMBigInteger left  = (pVMBigInteger)frame->Pop();
    
    CHECK_BIGINT(rightObj, right);   
    
    // Do operation and perform conversion to Integer if required
    pVMBigInteger result = _UNIVERSE->NewBigInteger(  left->GetEmbeddedInteger()
                                                    & right->GetEmbeddedInteger());
    
    frame->Push((pVMObject) result);
}


void  _BigInteger::Equal(pVMObject /*object*/, pVMFrame frame) {
    pVMObject rightObj  = frame->Pop();
    pVMBigInteger right = NULL;
    pVMBigInteger left  = (pVMBigInteger)frame->Pop();
    
    CHECK_BIGINT(rightObj, right);   
    
    // Do operation:
    if(left->GetEmbeddedInteger() == right->GetEmbeddedInteger())
        frame->Push(trueObject);
    else
        frame->Push(falseObject);
}


void  _BigInteger::Lowerthan(pVMObject /*object*/, pVMFrame frame) {
    pVMObject rightObj  = frame->Pop();
    pVMBigInteger right = NULL;
    pVMBigInteger left  = (pVMBigInteger)frame->Pop();
    
    CHECK_BIGINT(rightObj, right);   
    
    // Do operation:
    if(left->GetEmbeddedInteger() < right->GetEmbeddedInteger())
        frame->Push(trueObject);
    else
        frame->Push(falseObject);
     
}


void  _BigInteger::AsString(pVMObject /*object*/, pVMFrame frame) {
    pVMBigInteger self = (pVMBigInteger)frame->Pop();

    int64_t bigint = self->GetEmbeddedInteger();
    ostringstream Str;
    Str << bigint;
    frame->Push((pVMObject)_UNIVERSE->NewString(Str.str().c_str()));
    
}


void  _BigInteger::Sqrt(pVMObject /*object*/, pVMFrame frame) {
    pVMBigInteger self = (pVMBigInteger)frame->Pop();
    int64_t i = self->GetEmbeddedInteger();
    frame->Push((pVMObject)_UNIVERSE->NewDouble(sqrt((double)i)));
}


