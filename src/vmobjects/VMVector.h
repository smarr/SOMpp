#pragma once

#include <cstddef>

#include "../vmobjects/VMInteger.h"
#include "../vmobjects/VMObject.h"
#include "VMArray.h"

class VMVector : public VMObject {
public:
    typedef GCVector Stored;

    explicit VMVector(vm_oop_t first, vm_oop_t last, VMArray* storage);

    [[nodiscard]] inline vm_oop_t GetIndexableField(size_t index) {
        int64_t const first = INT_VAL(load_ptr(this->first));
        int64_t const last = INT_VAL(load_ptr(this->last));
        VMArray* const storage = load_ptr(this->storage);
        if (index < 1 || index > first + last) {
            IndexOutOfBounds(index, (last - first));
            // TODO(smarr): check if this would be correct
        }
        vm_oop_t returned = storage->GetIndexableField(first + index - 2);
        return returned;
    }

    static __attribute__((noreturn)) __attribute__((noinline)) void
    IndexOutOfBounds(size_t idx, size_t size);

private:
    static const size_t VMVectorNumberOfFields;

    gc_oop_t first;
    gc_oop_t last;
    GCArray* storage;
};
