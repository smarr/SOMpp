#pragma once

#ifndef VMINTPOINTER_H_
#define VMINTPOINTER_H_

#include "VMPointer.h"
#include "VMInteger.h"

/**
 * max value for tagged integers
 * 01111111 11111111 11111111 1111111X  
 */
#define VMTAGGEDINTEGER_MAX  1073741823

/**
 * min value for tagged integers
 * 10000000 00000000 00000000 0000000X  
 */
#define VMTAGGEDINTEGER_MIN -1073741824

//Derived smart pointer class that implements tagged integers.
//Implemented as a derived class to avoid ambiguity in type
//conversion operators and to avoid having to include
//VMInteger in the original smart pointer.
class VMIntPointer : public VMPointer<VMInteger>  {
public:
    //
    //constructors:
    //Empty constructor, creates NULL pointer
    VMIntPointer() : VMPointer<VMInteger>() { pointer = NULL; };
    //Initialize with VMInteger object
    VMIntPointer(VMInteger* o) : VMPointer<VMInteger>(o) {};
    
    template <class T>
    VMIntPointer(VMPointer<T> ptr) : VMPointer<VMInteger>(ptr) {};

    //int initialization constructor
    VMIntPointer(int32_t val) : VMPointer<VMInteger>() { SetIntegerValue(val); };
    
    //implicit conversion to int - either unmarshals the tagged integer or calls GetEmbeddedInteger()
    operator int32_t() const{
        return GetIntegerValue();
    };

    //Assignment from int
    inline VMIntPointer& operator =(const int32_t value){
        //if (pointer) pointer->DecreaseGCCount();
        this->SetIntegerValue(value);
        return *this;
    };
    
    inline bool operator==(const int32_t val) {
        return GetIntegerValue() == val;
    };
    
    inline bool operator!=(const int32_t val) {
        return GetIntegerValue() != val;
    };


private:
    inline bool IsTaggedInteger() const{
        return (((int32_t)pointer & 1) != 0);
    };

    inline int32_t GetIntegerValue() const{ 
        if (IsTaggedInteger())  //lowest bit set?
        {
            //clear lowest bit and return int
            return (int32_t)((int32_t)pointer >> 1);
        }
        //it's a VMInteger object, so we need call GetEmbeddedInteger().
        return pointer->GetEmbeddedInteger();
        
    };

    void SetIntegerValue(int32_t value){
        // Check with tagged integer border
        if(value < VMTAGGEDINTEGER_MIN || value > VMTAGGEDINTEGER_MAX ) {
            //it's too big, so we need an actual Object
            pointer = _UNIVERSE->NewInteger(value);
            //pointer->IncreaseGCCount();
        }
        else 
            pointer = (VMInteger*) (1 | value << 1);
    };
    

};


#endif VMINTPOINTER_H_

