#pragma once

#include "VMFrame.h"

inline void VMBlock::SetContext(VMFrame* contxt) {
    store_ptr(context, contxt);
}

inline VMFrame* VMBlock::GetContext() {
    return load_ptr(context);
}
