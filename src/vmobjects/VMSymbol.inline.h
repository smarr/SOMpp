#pragma once

#include "VMClass.h"
#include "VMInvokable.h"


VMInvokable* VMSymbol::GetCachedInvokable(VMClass* cls) {
# warning TODO: figure out whether we can do something for the pauseless GC
    if (GC_TYPE == PAUSELESS)  // not supported in PAUSELESS
        return nullptr;
    
# warning TODO: make thread-safe
    if (cls == load_ptr(cachedClass_invokable[0]))
        return load_ptr(cachedInvokable[0]);
    else if (cls == load_ptr(cachedClass_invokable[1]))
        return load_ptr(cachedInvokable[1]);
    else if (cls == load_ptr(cachedClass_invokable[2]))
        return load_ptr(cachedInvokable[2]);
    return nullptr;
}

void VMSymbol::UpdateCachedInvokable(VMClass* cls, VMInvokable* invo) {
    store_ptr(cachedInvokable[nextCachePos], invo);
    store_ptr(cachedClass_invokable[nextCachePos], cls);
    nextCachePos = (nextCachePos + 1) % 3;
}
