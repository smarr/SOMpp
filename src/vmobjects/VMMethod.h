#pragma once
#ifndef VMMETHOD_H_
#define VMMETHOD_H_

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

class VMArray;
class VMObject;
class VMInteger;
class MethodGenerationContext;
class VMFrame;

class VMMethod: public VMInvokable {

public:
    VMMethod(long bcCount, long numberOfConstants, long nof = 0);

    inline  long      GetNumberOfLocals() const;
            void      SetNumberOfLocals(long nol);
            long      GetMaximumNumberOfStackElements() const;
            void      SetMaximumNumberOfStackElements(long stel);
    inline  long      GetNumberOfArguments() const;
            void      SetNumberOfArguments(long);
            long      GetNumberOfBytecodes() const;
            void      SetHolderAll(pVMClass hld);
            pVMObject GetConstant(long indx) const;
    inline  uint8_t   GetBytecode(long indx) const;
    inline  void      SetBytecode(long indx, uint8_t);
    virtual void      MarkReferences();
    inline  long      GetNumberOfIndexableFields() const;
    inline  void      SetIndexableField(long idx, pVMObject item);

    //VMArray Methods....

    pVMArray CopyAndExtendWith(pVMObject) const;
    void     CopyIndexableFieldsTo(pVMArray) const;

    //-----------VMInvokable-------------//
    //operator "()" to invoke the method
    virtual void operator()(pVMFrame frame);

    virtual void SetSignature(pVMSymbol sig);

private:
    pVMObject GetIndexableField(long idx) const;

    pVMInteger numberOfLocals;
    pVMInteger maximumNumberOfStackElements;
    pVMInteger bcLength;
    pVMInteger numberOfArguments;
    pVMInteger numberOfConstants;

    static const long VMMethodNumberOfFields;
};

inline long VMMethod::GetNumberOfLocals() const {
    return numberOfLocals->GetEmbeddedInteger();
}

long VMMethod::GetNumberOfIndexableFields() const {
    //cannot be done using GetAdditionalSpaceConsumption,
    //as bytecodes need space, too, and there might be padding
    return numberOfConstants->GetEmbeddedInteger();
}


inline long VMMethod::GetNumberOfArguments() const {
    return numberOfArguments->GetEmbeddedInteger();
}

#define theEntries(i) FIELDS[this->GetNumberOfFields()+i]

void VMMethod::SetIndexableField(long idx, pVMObject item) {
    if (idx > GetNumberOfIndexableFields() - 1 || idx < 0) {
        cout << "Array index out of bounds: Accessing " << idx
        << ", but there is only space for "
        << this->GetNumberOfIndexableFields()
        << " entries available\n";
        _UNIVERSE->ErrorExit("Array index out of bounds exception");
    }
    theEntries(idx) = item;
}

#undef theEntries

#define _BC ((uint8_t*)&FIELDS[this->GetNumberOfFields() + this->GetNumberOfIndexableFields()])

uint8_t VMMethod::GetBytecode(long indx) const {
    return _BC[indx];
}

void VMMethod::SetBytecode(long indx, uint8_t val) {
    _BC[indx] = val;
}

#undef _BC

#endif
