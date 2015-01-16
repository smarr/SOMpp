#pragma once

void VMBlock::SetContext(VMFrame* contxt) {
    context = WRITEBARRIER(contxt);
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, (AbstractVMObject*)contxt);
#endif
}

VMFrame* VMBlock::GetContext() {
    return load_ptr(context);
}

