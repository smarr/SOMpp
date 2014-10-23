#pragma once
#ifndef VMARRAY_H_
#define VMARRAY_H_

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

#include "VMObject.h"
#include "VMInteger.h"

class VMArray: public VMObject {
public:
    VMArray(long size, long nof = 0);

#if GC_TYPE==PAUSELESS
    virtual pVMArray Clone(BaseThread*);
    virtual void MarkReferences();
#else
    virtual pVMArray Clone();
    virtual void WalkObjects(VMOBJECT_PTR (VMOBJECT_PTR));    
#endif

    inline  long GetNumberOfIndexableFields() const;
    pVMArray CopyAndExtendWith(pVMObject);
    pVMObject GetIndexableField(long idx);
    void SetIndexableField(long idx, pVMObject value);
    void CopyIndexableFieldsTo(pVMArray);

private:
    virtual void MarkObjectAsInvalid();
    
    static const long VMArrayNumberOfFields;
};

long VMArray::GetNumberOfIndexableFields() const {
    long numIndexableFields = GetAdditionalSpaceConsumption() / sizeof(pVMObject);
    /*
    static const pVMArray cachedArray = NULL;
    static long numIndexableFields = -1;

    if (this != cachedArray) {
        numIndexableFields = GetAdditionalSpaceConsumption() / sizeof(pVMObject);
        cachedArray = this;
    }*/
    return numIndexableFields;
}

#endif
