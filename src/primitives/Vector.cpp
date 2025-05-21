#include "Vector.h"

#include <cstdint>

#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMVector.h"

static vm_oop_t vecNew(vm_oop_t /*unused*/, vm_oop_t arg) {
    int64_t const size = INT_VAL(arg);
    return Universe::NewVector(size);
}

static vm_oop_t vecAt(vm_oop_t obj, vm_oop_t arg) {
    auto* self = static_cast<VMVector*>(obj);
    int64_t const index = INT_VAL(arg);
    return self->GetIndexableField(index);
}

_Vector::_Vector() {
    Add("new:", &vecNew, true);
    Add("at:", &vecAt, false);
}