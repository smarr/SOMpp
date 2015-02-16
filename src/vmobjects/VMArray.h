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
    
    VMArray(size_t size, size_t numberOfGcPtrFields = 0);

    virtual void WalkObjects(walk_heap_fn, Page*);

    inline  size_t GetNumberOfIndexableFields() const;
    VMArray* CopyAndExtendWith(vm_oop_t, Page*);
    vm_oop_t GetIndexableField(long idx);
    void SetIndexableField(long idx, vm_oop_t value);
    void CopyIndexableFieldsTo(VMArray*);
    virtual VMArray* Clone(Page*);
    
    virtual StdString AsDebugString();

    virtual void MarkObjectAsInvalid();

private:
    static const size_t VMArrayNumberOfGcPtrFields;
    
    // returns the arrays additional memory used
    size_t getAdditionalSpaceConsumption() const {
        //The VMArrays's additional memory used needs to be calculated.
        //It's the total object size   MINUS   sizeof(VMArray),
        // MINUS   the number of fields times sizeof(gc_oop_t)
        //   -> currently, in SOM it is not clear whether arrays should have
        //      fields, in addition to the indexable fields,
        //      but it is possible in SOM++
        return objectSize - (sizeof(VMArray)
                   + sizeof(gc_oop_t) * GetNumberOfFields());
    }
};

size_t VMArray::GetNumberOfIndexableFields() const {
    return getAdditionalSpaceConsumption() / sizeof(gc_oop_t);
}
