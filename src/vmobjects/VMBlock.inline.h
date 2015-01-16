#pragma once

void VMBlock::SetContext(VMFrame* contxt) {
    context = store_ptr(contxt);
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, (AbstractVMObject*)contxt);
#endif
}

VMFrame* VMBlock::GetContext() {
    return load_ptr(context);
}

