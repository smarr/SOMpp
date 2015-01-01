#pragma once

/*
 *
 *
 Copyright (c) 2007 Michael Haupt, Tobias Pape, Arne Bergmann
 Software Architecture Group, Hasso Plattner Institute, Potsdam, Germany
 http://www.hpi.uni-potsdam.de/swa/

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

#include <iostream>

#include "VMInvokable.h"
#include "VMInteger.h"

class MethodGenerationContext;
class Interpreter;

class VMMethod: public VMInvokable {
    friend class Interpreter;

public:
    VMMethod(long bcCount, long numberOfConstants, long nof = 0);

    inline  long      GetNumberOfLocals() const;
            void      SetNumberOfLocals(long nol);
            long      GetMaximumNumberOfStackElements() const;
            void      SetMaximumNumberOfStackElements(long stel);
    inline  long      GetNumberOfArguments() const;
            void      SetNumberOfArguments(long);
            long      GetNumberOfBytecodes() const;
            void      SetHolderAll(VMClass* hld);
            oop_t GetConstant(long indx) const;
    inline  uint8_t   GetBytecode(long indx) const;
    inline  void      SetBytecode(long indx, uint8_t);
#ifdef UNSAFE_FRAME_OPTIMIZATION
    void SetCachedFrame(VMFrame* frame);
    VMFrame* GetCachedFrame() const;
#endif
    virtual void WalkObjects(oop_t (oop_t));
    inline  long      GetNumberOfIndexableFields() const;
    virtual VMMethod* Clone() const;

    inline  void      SetIndexableField(long idx, oop_t item);

    //-----------VMInvokable-------------//
    //operator "()" to invoke the method
    virtual void operator()(VMFrame* frame);

    void SetSignature(VMSymbol* sig);

private:
    inline uint8_t* GetBytecodes() const;
    inline oop_t GetIndexableField(long idx) const;

    VMInteger* numberOfLocals;
    VMInteger* maximumNumberOfStackElements;
    VMInteger* bcLength;
    VMInteger* numberOfArguments;
    VMInteger* numberOfConstants;
#ifdef UNSAFE_FRAME_OPTIMIZATION
    VMFrame* cachedFrame;
#endif
    oop_t* indexableFields;
    uint8_t* bytecodes;
    static const long VMMethodNumberOfFields;
};

inline long VMMethod::GetNumberOfLocals() const {
    return INT_VAL(numberOfLocals);
}

long VMMethod::GetNumberOfIndexableFields() const {
    //cannot be done using GetAdditionalSpaceConsumption,
    //as bytecodes need space, too, and there might be padding
    return INT_VAL(numberOfConstants);
}

uint8_t* VMMethod::GetBytecodes() const {
    return bytecodes;
}

inline long VMMethod::GetNumberOfArguments() const {
    return INT_VAL(numberOfArguments);
}

oop_t VMMethod::GetIndexableField(long idx) const {
    return indexableFields[idx];
}

void VMMethod::SetIndexableField(long idx, oop_t item) {
    indexableFields[idx] = item;
    write_barrier(this, item);
}

uint8_t VMMethod::GetBytecode(long indx) const {
    return bytecodes[indx];
}

void VMMethod::SetBytecode(long indx, uint8_t val) {
    bytecodes[indx] = val;
}
