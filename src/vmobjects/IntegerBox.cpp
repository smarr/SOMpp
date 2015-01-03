#include "IntegerBox.h"
#include "VMInteger.h"
#include "../vm/Universe.h"

GCInteger* GlobalBox::integerBox = nullptr;

void GlobalBox::updateIntegerBox(VMInteger* newValue) {
# warning Is this acceptable use of _store_ptr??
    integerBox = _store_ptr(newValue);
}

VMInteger* GlobalBox::IntegerBox() {
    return load_ptr(integerBox);
}

void GlobalBox::WalkGlobals(walk_heap_fn walk) {
    integerBox = static_cast<GCInteger*>(walk(integerBox));
}
