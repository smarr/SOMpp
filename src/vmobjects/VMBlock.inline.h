#pragma once

void VMBlock::SetContext(pVMFrame contxt) {
    context = WRITEBARRIER(contxt);
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, (AbstractVMObject*)contxt);
#endif
}

pVMFrame VMBlock::GetContext() {
    return READBARRIER(context);
}

