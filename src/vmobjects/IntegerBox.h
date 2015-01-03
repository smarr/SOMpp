#pragma once

#include <vmobjects/ObjectFormats.h>

class GlobalBox {
public:
    static VMInteger* IntegerBox();
    
    static void WalkGlobals(walk_heap_fn walk);

private:
    static void updateIntegerBox(VMInteger*);
    static GCInteger* integerBox;
    friend class Universe;
};
