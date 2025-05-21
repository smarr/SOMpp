#pragma once

#include <cstddef>

#include "../vmobjects/VMInteger.h"
#include "../vmobjects/VMObject.h"
#include "VMArray.h"

class VMVector : public VMObject {
public:
    typedef GCVector Stored;

    explicit VMVector(size_t vectorSize, size_t additionalBytes)
        : VMObject(vectorSize +
                  VMVectorNumberOfFields, 
                  additionalBytes + sizeof(VMVector)) {
        assert(VMVectorNumberOfFields == 3);
    }

    [[nodiscard]] inline vm_oop_t GetIndexableField(size_t index) {
        VMArray* storage = GetFieldArray(this);
        VMInteger* first = GetFieldFirst(this);
        VMInteger* last = GetFieldLast(this);
        if (index < first->GetEmbeddedInteger() || index > last->GetEmbeddedInteger()) {
            IndexOutOfBounds(index);
            // TODO: check if this would be correct
        }
        vm_oop_t returned = storage->GetIndexableField(first->GetEmbeddedInteger()+index-1);
        return returned;
    }

    VMArray* GetFieldArray(vm_oop_t) const;
    VMInteger* GetFieldFirst(vm_oop_t) const;
    VMInteger* GetFieldLast(vm_oop_t) const;

    __attribute__((noreturn)) __attribute__((noinline)) void IndexOutOfBounds(
    size_t idx) const;


    private:
        static const size_t VMVectorNumberOfFields;
};