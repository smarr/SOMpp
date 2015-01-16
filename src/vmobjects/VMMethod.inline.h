#pragma once

inline long VMMethod::GetNumberOfLocals() {
    return INT_VAL(READBARRIER(numberOfLocals));
}

long VMMethod::GetNumberOfIndexableFields() {
    //cannot be done using GetAdditionalSpaceConsumption,
    //as bytecodes need space, too, and there might be padding
    return INT_VAL(READBARRIER(this->numberOfConstants));
}

uint8_t* VMMethod::GetBytecodes() const {
    return bytecodes;
}

inline long VMMethod::GetNumberOfArguments() {
    return INT_VAL(READBARRIER(numberOfArguments));
}

#if GC_TYPE==PAUSELESS
inline  long VMMethod::GetNumberOfArgumentsGC() {
    return INT_VAL(ReadBarrierForGCThread(&numberOfArguments));
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
