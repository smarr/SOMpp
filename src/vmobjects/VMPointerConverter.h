#pragma once

#ifndef VMPOINTERCONVERTER_H_
#define VMPOINTERCONVERTER_H_

#include "VMPointer.h"
#include "VMIntPointer.h"
#include "VMInteger.h"
//This template fucntion converts smart pointer of one type into
//smart pointers of a different type using type checking.
template<class TO, class FROM>
static VMPointer<TO> DynamicConvert(VMPointer<FROM> pu) {
    if ((((size_t)pu.GetPointer()) & 1) != 0) return VMPointer<TO>(NULL);
    TO* ptr = dynamic_cast<TO*>(pu.GetPointer());
    return VMPointer<TO>(ptr);
};

template<class FROM>
static VMIntPointer ConvertToInteger(VMPointer<FROM> pu) {
    if ((((size_t)pu.GetPointer()) & 1) != 0) 
        return VMIntPointer((VMInteger*)pu.GetPointer());
    VMInteger* ptr = dynamic_cast<VMInteger*>(pu.GetPointer());
    
    return VMIntPointer(ptr);
};


#endif

