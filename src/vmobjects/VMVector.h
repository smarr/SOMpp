#pragma once

#include <cstddef>

#include "../vm/Universe.h"
#include "../vmobjects/VMInteger.h"
#include "../vmobjects/VMObject.h"
#include "ObjectFormats.h"
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

    [[nodiscard]] inline vm_oop_t GetFirst() {
        int64_t first = INT_VAL(load_ptr(this->first));
        vm_oop_t returned = GetIndexableField(1);
        return returned;
    }

    [[nodiscard]] inline vm_oop_t GetLast() {
        int64_t last = INT_VAL(load_ptr(this->last));
        int64_t first = INT_VAL(load_ptr(this->first));
        vm_oop_t returned = GetIndexableField(last - 1);
        return returned;
    }

    inline void SetIndexableField(size_t index, vm_oop_t value) {
        int64_t const first = INT_VAL(load_ptr(this->first));
        int64_t const last = INT_VAL(load_ptr(this->last));
        VMArray* const storage = load_ptr(this->storage);
        if (index < 1 || index > first + last) {
            IndexOutOfBounds(index, (last - first));
            // TODO(smarr): check if this would be correct
        }
        storage->SetIndexableField(first + index - 2, value);
    }

    inline void Append(vm_oop_t value) {
        int64_t first = INT_VAL(load_ptr(this->first));
        int64_t last = INT_VAL(load_ptr(this->last));
        VMArray* storage = load_ptr(this->storage);

        if (last >=
            storage->GetNumberOfIndexableFields()) {  // Expand the array
            // Expand by the correct amount *2 by default in the native som
            // implementation
            VMArray* newStorage =
                Universe::NewArray(storage->GetNumberOfIndexableFields() * 2);

            storage->CopyIndexableFieldsTo(newStorage);
            newStorage->SetIndexableField(last - 1, value);

            SetField(2 /* storage */, newStorage);

        } else {  // Just set the new value
            storage->SetIndexableField(last - 1, value);
        }

        last += 1;
        this->last = store_ptr(this->last, NEW_INT(last));
    }

    static __attribute__((noreturn)) __attribute__((noinline)) void
    IndexOutOfBounds(size_t idx, size_t size);

private:
    static const size_t VMVectorNumberOfFields;

    gc_oop_t first;
    gc_oop_t last;
    GCArray* storage;
};
