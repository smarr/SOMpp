#pragma once

#include <vmobjects/ObjectFormats.h>
#include <memory/PauselessHeap.h>

inline VMFrame* Interpreter::GetFrame() {
    return load_ptr(this->frame);
}
