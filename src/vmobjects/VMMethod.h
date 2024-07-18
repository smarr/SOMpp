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

#include "../vm/Print.h"
#include "VMInteger.h"
#include "VMInvokable.h"

class MethodGenerationContext;
class Interpreter;

class VMMethod: public VMInvokable {
    friend class Interpreter;
    friend class Disassembler;

public:
    typedef GCMethod Stored;
    
    VMMethod(long bcCount, long numberOfConstants);

    inline long GetNumberOfLocals() const {
        return INT_VAL(load_ptr(numberOfLocals));
    }
    
            void      SetNumberOfLocals(long nol);
            long      GetMaximumNumberOfStackElements() const;
            void      SetMaximumNumberOfStackElements(long stel);
    
    inline long GetNumberOfArguments() const {
        return INT_VAL(load_ptr(numberOfArguments));
    }
    
            void      SetNumberOfArguments(long);
            long      GetNumberOfBytecodes() const;
    virtual void      SetHolder(VMClass* hld);
            void      SetHolderAll(VMClass* hld);
    
            inline vm_oop_t GetConstant(long indx) const {
                const uint8_t bc = bytecodes[indx + 1];
                if (bc >= GetNumberOfIndexableFields()) {
                    ErrorPrint("Error: Constant index out of range\n");
                    return nullptr;
                }
                return GetIndexableField(bc);
            }
    
    inline  uint8_t   GetBytecode(long indx) const;
    inline  void      SetBytecode(long indx, uint8_t);
#ifdef UNSAFE_FRAME_OPTIMIZATION
    void SetCachedFrame(VMFrame* frame);
    VMFrame* GetCachedFrame() const;
#endif
    virtual void WalkObjects(walk_heap_fn);
    
    inline  long GetNumberOfIndexableFields() const {
        //cannot be done using GetAdditionalSpaceConsumption,
        //as bytecodes need space, too, and there might be padding
        return INT_VAL(load_ptr(numberOfConstants));
    }
    
    virtual VMMethod* Clone() const;

    inline  void SetIndexableField(long idx, vm_oop_t item);

    virtual void Invoke(Interpreter* interp, VMFrame* frame);

    void SetSignature(VMSymbol* sig);
    
    virtual StdString AsDebugString() const;

private:
    inline uint8_t* GetBytecodes() const {
        return bytecodes;
    }
    
    inline vm_oop_t GetIndexableField(long idx) const {
        return load_ptr(indexableFields[idx]);
    }

private_testable:
    gc_oop_t numberOfLocals;
    gc_oop_t maximumNumberOfStackElements;
    gc_oop_t bcLength;
    gc_oop_t numberOfArguments;
    gc_oop_t numberOfConstants;

private:
#ifdef UNSAFE_FRAME_OPTIMIZATION
    GCFrame* cachedFrame;
#endif
    gc_oop_t* indexableFields;
    uint8_t* bytecodes;
    static const long VMMethodNumberOfFields;
};

void VMMethod::SetIndexableField(long idx, vm_oop_t item) {
    store_ptr(indexableFields[idx], item);
}

uint8_t VMMethod::GetBytecode(long indx) const {
    return bytecodes[indx];
}

void VMMethod::SetBytecode(long indx, uint8_t val) {
    bytecodes[indx] = val;
}
