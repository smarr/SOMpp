#pragma once

#include <misc/defs.h>

template <typename HeapClass>
class MemoryPage {
public:
    void* AllocateObject(size_t size ALLOC_OUTSIDE_NURSERY_DECL);
};
