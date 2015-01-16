#pragma once

inline long VMMethod::GetNumberOfLocals() {
#ifdef USE_TAGGING
    return UNTAG_INTEGER(numberOfLocals);
#else
    return READBARRIER(numberOfLocals)->GetEmbeddedInteger();
#endif
}

long VMMethod::GetNumberOfIndexableFields() {
    //cannot be done using GetAdditionalSpaceConsumption,
    //as bytecodes need space, too, and there might be padding
#ifdef USE_TAGGING
    return UNTAG_INTEGER(this->numberOfConstants);
#else
    return READBARRIER(this->numberOfConstants)->GetEmbeddedInteger();
#endif
}

uint8_t* VMMethod::GetBytecodes() const {
    return bytecodes;
}

inline long VMMethod::GetNumberOfArguments() {
#ifdef USE_TAGGING
    return UNTAG_INTEGER(numberOfArguments);
#else
    return READBARRIER(numberOfArguments)->GetEmbeddedInteger();
#endif
}

#if GC_TYPE==PAUSELESS
inline  long VMMethod::GetNumberOfArgumentsGC() {
    return ReadBarrierForGCThread(&numberOfArguments)->GetEmbeddedInteger();
}
#endif

vm_oop_t VMMethod::GetIndexableField(long idx) {
    return READBARRIER(indexableFields[idx]);
}

void VMMethod::SetIndexableField(long idx, VMObject* item) {
    indexableFields[idx] = WRITEBARRIER(item);
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, item);
#endif
}

uint8_t VMMethod::GetBytecode(long indx) const {
    return bytecodes[indx];
}

void VMMethod::SetBytecode(long indx, uint8_t val) {
    bytecodes[indx] = val;
}
