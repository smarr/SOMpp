#include "../vmobjects/VMVector.h"

#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>

#include "../misc/defs.h"
#include "../vm/Print.h"
#include "../vmobjects/ObjectFormats.h"

const size_t VMVector::VMVectorNumberOfFields = 3;

VMVector::VMVector(vm_oop_t first, vm_oop_t last, VMArray* storage)
    : VMObject(VMVectorNumberOfFields, sizeof(VMVector)),
      first(store_with_separate_barrier(first)),
      last(store_with_separate_barrier(last)),
      storage(store_with_separate_barrier(storage)) {
    static_assert(VMVectorNumberOfFields == 3);
    write_barrier(this, storage);
}

void VMVector::IndexOutOfBounds(size_t idx) {
    ErrorExit(("vector index out of bounds: Accessing " + to_string(idx) +
               ", but vector size is only " + "\n")
                  .c_str());
}
