#include "../vmobjects/VMVector.h"

#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>

#include "../memory/Heap.h"
#include "../misc/defs.h"
#include "../vm/Print.h"
#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMObject.h"

const size_t VMVector::VMVectorNumberOfFields = 3;

VMArray* VMVector::GetFieldArray(vm_oop_t obj) const {
    auto* self = static_cast<VMVector*>(obj);
    vm_oop_t rawStorage = self->GetField(2);
    VMArray* storage = static_cast<VMArray*>(rawStorage);
    return storage;
}

VMInteger* VMVector::GetFieldFirst(vm_oop_t obj) const {
    auto* self = static_cast<VMVector*>(obj);
    vm_oop_t rawStorage = self->GetField(0);
    VMInteger* storage = static_cast<VMInteger*>(rawStorage);
    return storage;
}   

VMInteger* VMVector::GetFieldLast(vm_oop_t obj) const {
    auto* self = static_cast<VMVector*>(obj);
    vm_oop_t rawStorage = self->GetField(1);
    VMInteger* storage = static_cast<VMInteger*>(rawStorage);
    return storage;
}

void VMVector::IndexOutOfBounds(size_t idx) const {
    ErrorExit(("vector index out of bounds: Accessing " + to_string(idx) +
               ", but vector size is only " + "\n")
                  .c_str());
}