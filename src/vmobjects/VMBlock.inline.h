#pragma once

void VMBlock::SetContext(VMFrame* contxt) {
    store_ptr(context, contxt);
}

VMFrame* VMBlock::GetContext() {
    return load_ptr(context);
}
