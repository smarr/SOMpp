#include "IntegerBox.h"
#include "VMInteger.h"

#include <vm/Universe.h>

GCInteger* GlobalBox::integerBox = nullptr;

void GlobalBox::updateIntegerBox(VMInteger* newValue) {
    integerBox = to_gc_ptr(newValue);
}

VMInteger* GlobalBox::IntegerBox() {
    return load_ptr(integerBox);
}

void GlobalBox::WalkGlobals(walk_heap_fn walk, Page* page) {
#warning is this #if necessary?
#if USE_TAGGING
    do_walk(integerBox);
#endif
}
