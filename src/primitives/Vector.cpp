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

static vm_oop_t vecAtPut(vm_oop_t obj, vm_oop_t at, vm_oop_t put) {
    auto* self = static_cast<VMVector*>(obj);  // Cast itself as a VMVector
    int64_t const index = INT_VAL(at);         // Set the index looking for
    // Call method to set the value at index. That deals with 1to0 indexing
    // conversion
    self->SetIndexableField(index, put);
    return put;  // Return the value that was set
}

static vm_oop_t vecAppend(vm_oop_t obj, vm_oop_t arg) {
    auto* self = static_cast<VMVector*>(obj);
    self->Append(arg);
    return self;
}

static vm_oop_t vecFirst(vm_oop_t obj) {
    auto* self = static_cast<VMVector*>(obj);
    return self->GetFirst();
}

static vm_oop_t vecLast(vm_oop_t obj) {
    auto* self = static_cast<VMVector*>(obj);
    return self->GetLast();
}

_Vector::_Vector() {
    Add("new:", &vecNew, true);
    Add("at:", &vecAt, false);
    Add("at:put:", &vecAtPut, false);
    Add("append:", &vecAppend, false);
    Add("first", &vecFirst, false);
    Add("last", &vecLast, false);
}