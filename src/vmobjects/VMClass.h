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

#include <vector>

#include "VMObject.h"

#include <misc/defs.h>

#if defined(_MSC_VER)   //Visual Studio
#include <windows.h> 
#include "../primitives/Core.h"
#endif

class ClassGenerationContext;

class VMClass: public VMObject {
public:
    typedef GCClass Stored;
    
    VMClass();
    VMClass(long numberOfFields);

    inline VMClass*     GetSuperClass() const;
    inline void         SetSuperClass(VMClass*);
    inline bool         HasSuperClass() const;
    inline VMSymbol*    GetName() const;
    inline void         SetName(VMSymbol*);
    inline VMArray*     GetInstanceFields() const;
    inline void         SetInstanceFields(VMArray*);
    inline VMArray*     GetInstanceInvokables() const;
           void         SetInstanceInvokables(VMArray*);
           long         GetNumberOfInstanceInvokables() const;
           VMInvokable* GetInstanceInvokable(long) const;
           void         SetInstanceInvokable(long, VMInvokable*);
           VMInvokable* LookupInvokable(VMSymbol*) const;
           long         LookupFieldIndex(VMSymbol*) const;
           bool         AddInstanceInvokable(VMInvokable*);
           void         AddInstancePrimitive(VMPrimitive*);
           VMSymbol*    GetInstanceFieldName(long)const;
           long         GetNumberOfInstanceFields() const;
           bool         HasPrimitives() const;
           void         LoadPrimitives(const vector<StdString>&);
    virtual VMClass*    Clone() const;
           void         WalkObjects(walk_heap_fn walk);
    
    virtual void MarkObjectAsInvalid();
    
    virtual StdString AsDebugString() const;

private:
    bool hasPrimitivesFor(const StdString& cl) const;
    void setPrimitives(const StdString& cname, bool classSide);
    long numberOfSuperInstanceFields() const;

    GCClass* superClass;
    GCSymbol* name;
    GCArray* instanceFields;
    GCArray* instanceInvokables;

    static const long VMClassNumberOfFields;
};

#include "VMSymbol.h"
#include "VMArray.h"

VMClass* VMClass::GetSuperClass() const {
    return load_ptr(superClass);
}

void VMClass::SetSuperClass(VMClass* sup) {
    store_ptr(superClass, sup);
}

VMSymbol* VMClass::GetName() const {
    return load_ptr(name);
}

void VMClass::SetName(VMSymbol* nam) {
    store_ptr(name, nam);
}

bool VMClass::HasSuperClass() const {
    assert(Universe::IsValidObject(load_ptr(superClass)));
    return load_ptr(superClass) != load_ptr(nilObject);
}

VMArray* VMClass::GetInstanceFields() const {
    return load_ptr(instanceFields);
}

void VMClass::SetInstanceFields(VMArray* instFields) {
    store_ptr(instanceFields, instFields);
}

VMArray* VMClass::GetInstanceInvokables() const {
    return load_ptr(instanceInvokables);
}
