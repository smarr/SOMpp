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

#include "Integer.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <string>

#include "../misc/ParseInteger.h"
#include "../vm/Globals.h"
#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMDouble.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMString.h"

//
// arithmetic operations
//

static double coerceDouble(vm_oop_t x);

#define doDoubleOpIfNeeded(leftInt, rightObj, op)            \
    {                                                        \
        VMClass* cl = CLASS_OF(rightObj);                    \
        if (cl == load_ptr(doubleClass)) {                   \
            double const leftDbl = (double)(leftInt);        \
            double const rightDbl = coerceDouble(rightObj);  \
            return Universe::NewDouble(leftDbl op rightDbl); \
        }                                                    \
    }

static vm_oop_t intPlus(vm_oop_t leftObj, vm_oop_t rightObj) {
    assert(CLASS_OF(leftObj) == load_ptr(integerClass) &&
           "The receiver should always be an int");

    int64_t const left = INT_VAL(leftObj);
    doDoubleOpIfNeeded(left, rightObj, +);

    int64_t const result = left + INT_VAL(rightObj);
    return NEW_INT(result);
}

static vm_oop_t intBitwiseXor(vm_oop_t leftObj, vm_oop_t rightObj) {
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    int64_t const result = INT_VAL(leftObj) ^ INT_VAL(rightObj);
    return NEW_INT(result);
}

static vm_oop_t intLeftShift(vm_oop_t leftObj, vm_oop_t rightObj) {
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    int64_t const result = INT_VAL(leftObj) << INT_VAL(rightObj);
    return NEW_INT(result);
}

static vm_oop_t intUnsignedRightShift(vm_oop_t leftObj, vm_oop_t rightObj) {
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    int64_t const result = INT_VAL(leftObj) >> INT_VAL(rightObj);
    return NEW_INT(result);
}

static vm_oop_t intMinus(vm_oop_t leftObj, vm_oop_t rightObj) {
    int64_t const left = INT_VAL(leftObj);
    doDoubleOpIfNeeded(left, rightObj, -);

    int64_t const result = left - INT_VAL(rightObj);
    return NEW_INT(result);
}

static vm_oop_t intStar(vm_oop_t leftObj, vm_oop_t rightObj) {
    int64_t const left = INT_VAL(leftObj);
    doDoubleOpIfNeeded(left, rightObj, *);

    int64_t const result = left * INT_VAL(rightObj);
    return NEW_INT(result);
}

static vm_oop_t intSlashslash(vm_oop_t leftObj, vm_oop_t rightObj) {
    int64_t const left = INT_VAL(leftObj);
    doDoubleOpIfNeeded(left, rightObj, /);

    double const result = (double)left / (double)INT_VAL(rightObj);
    return Universe::NewDouble(result);
}

static vm_oop_t intSlash(vm_oop_t leftObj, vm_oop_t rightObj) {
    int64_t const left = INT_VAL(leftObj);
    int64_t right = 0;

    VMClass* cl = CLASS_OF(rightObj);
    if (cl == load_ptr(doubleClass)) {
        right = (int64_t)((VMDouble*)rightObj)->GetEmbeddedDouble();
    } else {
        right = INT_VAL(rightObj);
    }

    int64_t const result = left / right;
    return NEW_INT(result);
}

static vm_oop_t intPercent(vm_oop_t leftObj, vm_oop_t rightObj) {
    int64_t const l = INT_VAL(leftObj);
    int64_t r = 0;

    VMClass* cl = CLASS_OF(rightObj);
    if (cl == load_ptr(doubleClass)) {
        r = (int64_t)((VMDouble*)rightObj)->GetEmbeddedDouble();
    } else {
        r = INT_VAL(rightObj);
    }

    int64_t result = l % r;

    if ((result != 0) && ((result < 0) != (r < 0))) {
        result += r;
    }

    return NEW_INT(result);
}

static vm_oop_t dblPercent(vm_oop_t leftPtr, vm_oop_t rightObj);

static vm_oop_t intRem(vm_oop_t leftObj, vm_oop_t rightObj) {
    VMClass* cl = CLASS_OF(rightObj);
    if (cl == load_ptr(doubleClass)) {
        return dblPercent(leftObj, rightObj);
    }

    auto const l = INT_VAL(leftObj);
    auto const r = INT_VAL(rightObj);

    int64_t const result = l - (l / r) * r;

    return NEW_INT(result);
}

static vm_oop_t intAnd(vm_oop_t leftObj, vm_oop_t rightObj) {
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    int64_t const result = INT_VAL(leftObj) & INT_VAL(rightObj);
    return NEW_INT(result);
}

static vm_oop_t intEqual(vm_oop_t leftObj, vm_oop_t rightObj) {
    int64_t const left = INT_VAL(leftObj);

    if (IS_TAGGED(rightObj) || CLASS_OF(rightObj) == load_ptr(integerClass)) {
        if (left == INT_VAL(rightObj)) {
            return load_ptr(trueObject);
        }
    } else if (CLASS_OF(rightObj) == load_ptr(doubleClass)) {
        if ((double)left == ((VMDouble*)rightObj)->GetEmbeddedDouble()) {
            return load_ptr(trueObject);
        }
    }
    return load_ptr(falseObject);
}

static vm_oop_t intEqualEqual(vm_oop_t leftObj, vm_oop_t rightObj) {
    if (IS_TAGGED(rightObj) || CLASS_OF(rightObj) == load_ptr(integerClass)) {
        if (INT_VAL(leftObj) == INT_VAL(rightObj)) {
            return load_ptr(trueObject);
        }
    }
    return load_ptr(falseObject);
}

static vm_oop_t intLowerthan(vm_oop_t leftObj, vm_oop_t rightObj) {
    int64_t const left = INT_VAL(leftObj);
    doDoubleOpIfNeeded(left, rightObj, <);

    if (left < INT_VAL(rightObj)) {
        return load_ptr(trueObject);
    }
    return load_ptr(falseObject);
}

static vm_oop_t intLowerThanEqual(vm_oop_t leftObj, vm_oop_t rightObj) {
    int64_t const left = INT_VAL(leftObj);
    doDoubleOpIfNeeded(left, rightObj, <=);

    if (left <= INT_VAL(rightObj)) {
        return load_ptr(trueObject);
    }
    return load_ptr(falseObject);
}

static vm_oop_t intGreaterThan(vm_oop_t leftObj, vm_oop_t rightObj) {
    int64_t const left = INT_VAL(leftObj);
    doDoubleOpIfNeeded(left, rightObj, >);

    if (left > INT_VAL(rightObj)) {
        return load_ptr(trueObject);
    }
    return load_ptr(falseObject);
}

static vm_oop_t intGreaterThanEqual(vm_oop_t leftObj, vm_oop_t rightObj) {
    int64_t const left = INT_VAL(leftObj);
    doDoubleOpIfNeeded(left, rightObj, >=);

    if (left >= INT_VAL(rightObj)) {
        return load_ptr(trueObject);
    }
    return load_ptr(falseObject);
}

static vm_oop_t intAsString(vm_oop_t self) {
    int64_t const integer = INT_VAL(self);
    ostringstream Str;
    Str << integer;
    return Universe::NewString(Str.str());
}

static vm_oop_t intAsDouble(vm_oop_t self) {
    int64_t const integer = INT_VAL(self);
    return Universe::NewDouble((double)integer);
}

static vm_oop_t intAs32BitSigned(vm_oop_t self) {
    int64_t const integer = INT_VAL(self);
    return NEW_INT((int64_t)(int32_t)integer);
}

static vm_oop_t intAs32BitUnsigned(vm_oop_t self) {
    int64_t const integer = INT_VAL(self);
    return NEW_INT((int64_t)(uint32_t)integer);
}

static vm_oop_t intSqrt(vm_oop_t self) {
    double const result = sqrt((double)INT_VAL(self));

    if (result == rint(result)) {
        return NEW_INT((int64_t)result);
    }
    return Universe::NewDouble(result);
}

static vm_oop_t intAtRandom(vm_oop_t self) {
    int64_t const result =
        INT_VAL(self) *
        rand();  // NOLINT(clang-analyzer-security.insecureAPI.rand)
    return NEW_INT(result);
}

static vm_oop_t intAbs(vm_oop_t self) {
    int64_t const result = INT_VAL(self);
    if (result < 0) {
        return NEW_INT(-result);
    }
    return self;
}

static vm_oop_t intMin(vm_oop_t self, vm_oop_t arg) {
    int64_t const result = INT_VAL(self);

    VMClass* cl = CLASS_OF(arg);
    if (cl == load_ptr(doubleClass)) {
        if ((double)result < ((VMDouble*)arg)->GetEmbeddedDouble()) {
            return self;
        }
        return arg;
    }

    return (result < INT_VAL(arg)) ? self : arg;
}

static vm_oop_t intMax(vm_oop_t self, vm_oop_t arg) {
    int64_t const result = INT_VAL(self);

    VMClass* cl = CLASS_OF(arg);
    if (cl == load_ptr(doubleClass)) {
        if ((double)result > ((VMDouble*)arg)->GetEmbeddedDouble()) {
            return self;
        }
        return arg;
    }

    return (result > INT_VAL(arg)) ? self : arg;
}

static vm_oop_t intFromString(vm_oop_t /*unused*/, vm_oop_t right) {
    auto* self = (VMString*)right;
    std::string str = self->GetStdString();

    return ParseInteger(str, 10, false);
}

static vm_oop_t intUnequal(vm_oop_t leftObj, vm_oop_t rightObj) {
    int64_t const left = INT_VAL(leftObj);
    doDoubleOpIfNeeded(left, rightObj, !=);

    if (left != INT_VAL(rightObj)) {
        return load_ptr(trueObject);
    }
    return load_ptr(falseObject);
}

static vm_oop_t intRange(vm_oop_t leftObj, vm_oop_t rightObj) {
    int64_t const left = INT_VAL(leftObj);
    int64_t const right = INT_VAL(rightObj);

    int64_t const numInteger = right - left + 1;
    VMArray* arr = Universe::NewArray(numInteger);

    size_t index = 0;
    for (int64_t i = left; i <= right; i += 1, index += 1) {
        arr->SetIndexableField(index, NEW_INT(i));
    }

    return arr;
}

_Integer::_Integer() {
    srand((unsigned)time(nullptr));

    Add("+", &intPlus, false);
    Add("-", &intMinus, false);
    Add("*", &intStar, false);
    Add("rem:", &intRem, false);

    Add("bitXor:", &intBitwiseXor, false);
    Add("<<", &intLeftShift, false);
    Add(">>>", &intUnsignedRightShift, false);
    Add("/", &intSlash, false);
    Add("//", &intSlashslash, false);
    Add("%", &intPercent, false);
    Add("&", &intAnd, false);
    Add("=", &intEqual, false);
    Add("==", &intEqualEqual, false);
    Add("<", &intLowerthan, false);
    Add("asString", &intAsString, false);
    Add("asDouble", &intAsDouble, false);
    Add("as32BitSignedValue", &intAs32BitSigned, false);
    Add("as32BitUnsignedValue", &intAs32BitUnsigned, false);
    Add("sqrt", &intSqrt, false);
    Add("atRandom", &intAtRandom, false);
    Add("fromString:", &intFromString, true);

    Add("<=", &intLowerThanEqual, false);
    Add(">", &intGreaterThan, false);
    Add(">=", &intGreaterThanEqual, false);
    Add("<>", &intUnequal, false);
    Add("~=", &intUnequal, false);

    Add("abs", &intAbs, false);
    Add("min:", &intMin, false);
    Add("max:", &intMax, false);

    Add("to:", &intRange, false);
}
