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

#include "VMObject.h"
#include "VMInteger.h"

class VMArray: public VMObject {
public:
    typedef GCArray Stored;
    
    VMArray(long size, long nof = 0);

#if GC_TYPE==PAUSELESS
    virtual VMArray* Clone(Interpreter*);
    virtual VMArray* Clone(PauselessCollectorThread*);
    virtual void MarkReferences();
    virtual void CheckMarking(void (vm_oop_t));
#else
    virtual VMArray* Clone();
    virtual void WalkObjects(VMOBJECT_PTR (VMOBJECT_PTR));    
#endif

    inline  long GetNumberOfIndexableFields() const;
    VMArray* CopyAndExtendWith(vm_oop_t);
    vm_oop_t GetIndexableField(long idx);
    void SetIndexableField(long idx, vm_oop_t value);
    void CopyIndexableFieldsTo(VMArray*);
    
    virtual StdString AsDebugString();

    virtual void MarkObjectAsInvalid();

private:
    
    static const long VMArrayNumberOfFields;
};

long VMArray::GetNumberOfIndexableFields() const {
    long numIndexableFields = GetAdditionalSpaceConsumption() / sizeof(VMObject*);
    /*
    static const VMArray* cachedArray = nullptr;
    static long numIndexableFields = -1;

    if (this != cachedArray) {
        numIndexableFields = GetAdditionalSpaceConsumption() / sizeof(VMObject*);
        cachedArray = this;
    }*/
    return numIndexableFields;
}
