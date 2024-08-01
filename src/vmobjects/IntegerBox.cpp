#include "IntegerBox.h"

#include "ObjectFormats.h"
#include "VMInteger.h"

GCInteger* GlobalBox::integerBox = nullptr;

void GlobalBox::updateIntegerBox(VMInteger* newValue) {
    integerBox = store_root(newValue);
}

VMInteger* GlobalBox::IntegerBox() {
    return load_ptr(integerBox);
}

void GlobalBox::WalkGlobals(walk_heap_fn walk) {
    integerBox = static_cast<GCInteger*>(walk(integerBox));
}
