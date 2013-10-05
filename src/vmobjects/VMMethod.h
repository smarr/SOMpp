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

class VMArray;
class VMObject;
class VMInteger;
class MethodGenerationContext;
class VMFrame;

class VMMethod: public VMInvokable {

public:
    VMMethod(long bcCount, long numberOfConstants, long nof = 0);

    virtual long GetNumberOfLocals() const;
    virtual void SetNumberOfLocals(long nol);
    virtual long GetMaximumNumberOfStackElements() const;
    virtual void SetMaximumNumberOfStackElements(long stel);
    virtual long GetNumberOfArguments() const;
    virtual void SetNumberOfArguments(long);
    virtual long GetNumberOfBytecodes() const;
    virtual void SetHolderAll(pVMClass hld);
    virtual pVMObject GetConstant(long indx) const;
    virtual uint8_t GetBytecode(long indx) const;
    virtual void SetBytecode(long indx, uint8_t);
    virtual void MarkReferences();
    virtual long GetNumberOfIndexableFields() const;

    void SetIndexableField(long idx, pVMObject item);

    //VMArray Methods....

    pVMArray CopyAndExtendWith(pVMObject) const;
    void CopyIndexableFieldsTo(pVMArray) const;

    /// Methods are considered byte arrays with meta data.
    // So the index operator returns the bytecode at the index.
    // Not really used because it violates the C++ idiom to
    // implement operators in a "natural" way. Does not really
    // seem so natural to do this.
    uint8_t& operator[](long indx) const;

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

#endif
