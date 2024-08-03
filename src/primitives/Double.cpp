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

#include "Double.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

#include "../primitivesCore/PrimitiveContainer.h"
#include "../vm/Globals.h"
#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMDouble.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMInteger.h"
#include "../vmobjects/VMString.h"

/*
 * This function coerces any right-hand parameter to a double, regardless of its
 * true nature. This is to make sure that all Double operations return Doubles.
 */
double coerceDouble(vm_oop_t x) {
    if (IS_TAGGED(x)) {
        return (double)INT_VAL(x);
    }

    VMClass* cl = ((AbstractVMObject*)x)->GetClass();
    if (cl == load_ptr(doubleClass)) {
        return static_cast<VMDouble*>(x)->GetEmbeddedDouble();
    } else if (cl == load_ptr(integerClass)) {
        return (double)static_cast<VMInteger*>(x)->GetEmbeddedInteger();
    } else {
        GetUniverse()->ErrorExit(
            "Attempt to apply Double operation to non-number.");
    }

    return 0.0f;
}

/*
 * The following standard functionality is performed in all arithmetic
 * operations: extract the right-hand operand by coercing it to a double, and
 * extract the left-hand operand as an immediate Double. Afterwards, left and
 * right are prepared for the operation.
 */
#define PREPARE_OPERANDS                                 \
    double right = coerceDouble(rightObj);               \
    VMDouble* leftObj = static_cast<VMDouble*>(leftPtr); \
    double left = leftObj->GetEmbeddedDouble();

static vm_oop_t dblPlus(vm_oop_t leftPtr, vm_oop_t rightObj) {
    PREPARE_OPERANDS;
    return GetUniverse()->NewDouble(left + right);
}

static vm_oop_t dblMinus(vm_oop_t leftPtr, vm_oop_t rightObj) {
    PREPARE_OPERANDS;
    return GetUniverse()->NewDouble(left - right);
}

static vm_oop_t dblStar(vm_oop_t leftPtr, vm_oop_t rightObj) {
    PREPARE_OPERANDS;
    return GetUniverse()->NewDouble(left * right);
}

static vm_oop_t dblCos(vm_oop_t rcvr) {
    VMDouble* self = (VMDouble*)rcvr;
    double result = cos(self->GetEmbeddedDouble());
    return GetUniverse()->NewDouble(result);
}

static vm_oop_t dblSin(vm_oop_t rcvr) {
    VMDouble* self = (VMDouble*)rcvr;
    double result = sin(self->GetEmbeddedDouble());
    return GetUniverse()->NewDouble(result);
}

static vm_oop_t dblSlashslash(vm_oop_t leftPtr, vm_oop_t rightObj) {
    PREPARE_OPERANDS;
    return GetUniverse()->NewDouble(left / right);
}

vm_oop_t dblPercent(vm_oop_t leftPtr, vm_oop_t rightObj) {
    PREPARE_OPERANDS;
    return GetUniverse()->NewDouble((double)((int64_t)left % (int64_t)right));
}

/*
 * This function implements strict (bit-wise) equality and is therefore
 * inaccurate.
 */
static vm_oop_t dblEqual(vm_oop_t leftPtr, vm_oop_t rightObj) {
    PREPARE_OPERANDS;
    if (left == right) {
        return load_ptr(trueObject);
    } else {
        return load_ptr(falseObject);
    }
}

static vm_oop_t dblLowerthan(vm_oop_t leftPtr, vm_oop_t rightObj) {
    PREPARE_OPERANDS;
    if (left < right) {
        return load_ptr(trueObject);
    } else {
        return load_ptr(falseObject);
    }
}

static vm_oop_t dblAsString(vm_oop_t rcvr) {
    VMDouble* self = static_cast<VMDouble*>(rcvr);

    double dbl = self->GetEmbeddedDouble();
    ostringstream Str;
    Str.precision(17);
    Str << dbl;
    return GetUniverse()->NewString(Str.str().c_str());
}

static vm_oop_t dblSqrt(vm_oop_t rcvr) {
    VMDouble* self = static_cast<VMDouble*>(rcvr);
    VMDouble* result =
        GetUniverse()->NewDouble(sqrt(self->GetEmbeddedDouble()));
    return result;
}

static vm_oop_t dblRound(vm_oop_t rcvr) {
    VMDouble* self = (VMDouble*)rcvr;
    int64_t rounded = llround(self->GetEmbeddedDouble());

    return NEW_INT(rounded);
}

static vm_oop_t dblAsInteger(vm_oop_t rcvr) {
    VMDouble* self = (VMDouble*)rcvr;
    int64_t rounded = (int64_t)self->GetEmbeddedDouble();

    return NEW_INT(rounded);
}

static vm_oop_t dblPositiveInfinity(vm_oop_t) {
    return GetUniverse()->NewDouble(INFINITY);
}

static vm_oop_t dblFromString(vm_oop_t, vm_oop_t rightObj) {
    VMString* self = (VMString*)rightObj;
    double value =
        stod(std::string(self->GetRawChars(), self->GetStringLength()));
    return GetUniverse()->NewDouble(value);
}

_Double::_Double() : PrimitiveContainer() {
    Add("plus", &dblPlus, false);
    Add("minus", &dblMinus, false);
    Add("star", &dblStar, false);
    Add("cos", &dblCos, false);
    Add("sin", &dblSin, false);
    Add("slashslash", &dblSlashslash, false);
    Add("percent", &dblPercent, false);
    Add("equal", &dblEqual, false);
    Add("lowerthan", &dblLowerthan, false);
    Add("asString", &dblAsString, false);
    Add("sqrt", &dblSqrt, false);
    Add("round", &dblRound, false);
    Add("asInteger", &dblAsInteger, false);
    Add("PositiveInfinity", &dblPositiveInfinity, true);
    Add("fromString_", &dblFromString, true);
}
