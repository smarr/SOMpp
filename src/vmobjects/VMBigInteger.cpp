#include "VMBigInteger.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

#include "../memory/Heap.h"
#include "../misc/defs.h"
#include "../vm/Globals.h"
#include "../vm/Universe.h"
#include "../vmobjects/VMDouble.h"  // NOLINT(misc-include-cleaner)
#include "ObjectFormats.h"
#include "VMClass.h"

VMBigInteger* VMBigInteger::CloneForMovingGC() const {
    return new (GetHeap<HEAP_CLS>(), 0 ALLOC_MATURE) VMBigInteger(*this);
}

VMClass* VMBigInteger::GetClass() const {
    return load_ptr(integerClass);
}

std::string VMBigInteger::AsDebugString() const {
    return "Integer(" + embeddedInteger.toString() + ")";
}

#define INVALID_INT_MARKER 9002002002002002002LL

void VMBigInteger::MarkObjectAsInvalid() {
    embeddedInteger = InfInt(INVALID_INT_MARKER);
}

bool VMBigInteger::IsMarkedInvalid() const {
    return embeddedInteger == InfInt(INVALID_INT_MARKER);
}

vm_oop_t VMBigInteger::Add(int64_t value) const {
    InfInt const result = embeddedInteger + InfInt(value);
    // TODO(smarr): try to fit into SmallInt
    return new (GetHeap<HEAP_CLS>(), 0) VMBigInteger(std::move(result));
}

vm_oop_t VMBigInteger::Add(vm_oop_t value) const {
    if (IS_SMALL_INT(value)) {
        return Add(SMALL_INT_VAL(value));
    }

    if (IS_DOUBLE(value)) {
        double const left = embeddedInteger.toDouble();
        double const right = AS_DOUBLE(value);
        return Universe::NewDouble(left + right);
    }

    assert(IS_BIG_INT(value) && "assume rcvr is a big int now");

    InfInt const result = embeddedInteger + AS_BIG_INT(value)->embeddedInteger;
    // TODO(smarr): try to fit into SmallInt
    return new (GetHeap<HEAP_CLS>(), 0) VMBigInteger(std::move(result));
}

vm_oop_t VMBigInteger::SubtractFrom(int64_t value) const {
    InfInt const left = InfInt(value);
    InfInt const result = left - embeddedInteger;
    // TODO(smarr): try to fit into SmallInt
    return new (GetHeap<HEAP_CLS>(), 0) VMBigInteger(std::move(result));
}

vm_oop_t VMBigInteger::Subtract(vm_oop_t value) const {
    if (IS_SMALL_INT(value)) {
        InfInt const result = embeddedInteger - InfInt(SMALL_INT_VAL(value));
        return new (GetHeap<HEAP_CLS>(), 0) VMBigInteger(std::move(result));
    }

    if (IS_DOUBLE(value)) {
        double const left = embeddedInteger.toDouble();
        double const right = AS_DOUBLE(value);
        return Universe::NewDouble(left - right);
    }

    assert(IS_BIG_INT(value) && "assume rcvr is a big int now");

    InfInt const result = embeddedInteger - AS_BIG_INT(value)->embeddedInteger;
    // TODO(smarr): try to fit into SmallInt
    return new (GetHeap<HEAP_CLS>(), 0) VMBigInteger(std::move(result));
}

vm_oop_t VMBigInteger::Multiply(int64_t value) const {
    InfInt const result = embeddedInteger * InfInt(value);
    // TODO(smarr): try to fit into SmallInt
    return new (GetHeap<HEAP_CLS>(), 0) VMBigInteger(std::move(result));
}

vm_oop_t VMBigInteger::Multiply(vm_oop_t value) const {
    if (IS_SMALL_INT(value)) {
        InfInt const result = embeddedInteger * InfInt(SMALL_INT_VAL(value));
        return new (GetHeap<HEAP_CLS>(), 0) VMBigInteger(std::move(result));
    }

    if (IS_DOUBLE(value)) {
        double const left = embeddedInteger.toDouble();
        double const right = AS_DOUBLE(value);
        return Universe::NewDouble(left * right);
    }

    assert(IS_BIG_INT(value) && "assume rcvr is a big int now");

    InfInt const result = embeddedInteger * AS_BIG_INT(value)->embeddedInteger;
    // TODO(smarr): try to fit into SmallInt
    return new (GetHeap<HEAP_CLS>(), 0) VMBigInteger(std::move(result));
}

vm_oop_t VMBigInteger::DivisionFrom(int64_t value) const {
    InfInt const result = InfInt(value) / embeddedInteger;
    // TODO(smarr): try to fit into SmallInt
    return new (GetHeap<HEAP_CLS>(), 0) VMBigInteger(std::move(result));
}

vm_oop_t VMBigInteger::DivideBy(vm_oop_t value) const {
    if (IS_SMALL_INT(value)) {
        InfInt const result = embeddedInteger / InfInt(SMALL_INT_VAL(value));
        return new (GetHeap<HEAP_CLS>(), 0) VMBigInteger(std::move(result));
    }

    if (IS_DOUBLE(value)) {
        double const left = embeddedInteger.toDouble();
        double const right = AS_DOUBLE(value);
        return Universe::NewDouble(left / right);
    }

    assert(IS_BIG_INT(value) && "assume rcvr is a big int now");

    InfInt const result = embeddedInteger / AS_BIG_INT(value)->embeddedInteger;
    // TODO(smarr): try to fit into SmallInt
    return new (GetHeap<HEAP_CLS>(), 0) VMBigInteger(std::move(result));
}

vm_oop_t VMBigInteger::ModuloFrom(int64_t value) const {
    InfInt const result = InfInt(value) % embeddedInteger;
    // TODO(smarr): try to fit into SmallInt
    return new (GetHeap<HEAP_CLS>(), 0) VMBigInteger(std::move(result));
}

vm_oop_t VMBigInteger::Modulo(vm_oop_t value) const {
    if (IS_SMALL_INT(value)) {
        InfInt const result = embeddedInteger % InfInt(SMALL_INT_VAL(value));
        return new (GetHeap<HEAP_CLS>(), 0) VMBigInteger(std::move(result));
    }

    if (IS_DOUBLE(value)) {
        double const left = embeddedInteger.toDouble();
        double const right = AS_DOUBLE(value);
        return Universe::NewDouble(std::fmod(left, right));
    }

    assert(IS_BIG_INT(value) && "assume rcvr is a big int now");

    InfInt const result = embeddedInteger % AS_BIG_INT(value)->embeddedInteger;
    // TODO(smarr): try to fit into SmallInt
    return new (GetHeap<HEAP_CLS>(), 0) VMBigInteger(std::move(result));
}

vm_oop_t VMBigInteger::Negate() const {
    if (embeddedInteger < InfInt()) {
        return new (GetHeap<HEAP_CLS>(), 0) VMBigInteger(-embeddedInteger);
    }
    return const_cast<VMBigInteger*>(this);
}

vm_oop_t VMBigInteger::IsEqual(VMBigInteger* o) const {
    return embeddedInteger == o->embeddedInteger ? load_ptr(trueObject)
                                                 : load_ptr(falseObject);
}
