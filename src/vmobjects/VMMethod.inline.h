#pragma once

inline long VMMethod::GetNumberOfIndexableFields() const {
    //cannot be done using GetAdditionalSpaceConsumption,
    //as bytecodes need space, too, and there might be padding
    return INT_VAL(load_ptr(numberOfConstants));
}

inline uint8_t* VMMethod::GetBytecodes() const {
    return bytecodes;
}


inline long VMMethod::GetNumberOfArguments() const {
    return INT_VAL(load_ptr(numberOfArguments));
}

inline vm_oop_t VMMethod::GetIndexableField(long idx) const {
    return load_ptr(indexableFields[idx]);
}

inline void VMMethod::SetIndexableField(long idx, vm_oop_t item) {
    store_ptr(indexableFields[idx], item);
}

inline uint8_t VMMethod::GetBytecode(long indx) const {
    return bytecodes[indx];
}

inline void VMMethod::SetBytecode(long indx, uint8_t val) {
    bytecodes[indx] = val;
}
