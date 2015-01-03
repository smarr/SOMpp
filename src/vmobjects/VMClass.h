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
#include "VMSymbol.h"
#include "VMArray.h"

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
           void         SetInstanceInvokable(long, VMObject*);
           VMInvokable* LookupInvokable(VMSymbol*) const;
           long         LookupFieldIndex(VMSymbol*) const;
           bool         AddInstanceInvokable(VMObject*);
           void         AddInstancePrimitive(VMPrimitive*);
           VMSymbol*    GetInstanceFieldName(long)const;
           long         GetNumberOfInstanceFields() const;
           bool         HasPrimitives() const;
           void         LoadPrimitives(const vector<StdString>&);
    virtual VMClass*    Clone() const;
           void         WalkObjects(walk_heap_fn walk);
    
    virtual void MarkObjectAsInvalid();

private:
    StdString genLoadstring(const StdString& cp,
            const StdString& cname
    ) const;

    StdString genCoreLoadstring(const StdString& cp) const;

    void* loadLib(const StdString& path) const;
    bool isResponsible(void* handle, const StdString& cl) const;
    void setPrimitives(void* handle, const StdString& cname);
    long numberOfSuperInstanceFields() const;

    GCClass* superClass;
    GCSymbol* name;
    GCArray* instanceFields;
    GCArray* instanceInvokables;

    static const long VMClassNumberOfFields;
};

VMClass* VMClass::GetSuperClass() const {
    return superClass;
}

void VMClass::SetSuperClass(VMClass* sup) {
    superClass = sup;
    write_barrier(this, sup);
}

VMSymbol* VMClass::GetName() const {
    return name;
}

void VMClass::SetName(VMSymbol* nam) {
    name = nam;
    write_barrier(this, nam);
}

bool VMClass::HasSuperClass() const {
    assert(Universe::IsValidObject(superClass));
    return superClass != nilObject;
}

VMArray* VMClass::GetInstanceFields() const {
    return instanceFields;
}

void VMClass::SetInstanceFields(VMArray* instFields) {
    instanceFields = instFields;
    write_barrier(this, instFields);
}

VMArray* VMClass::GetInstanceInvokables() const {
    return instanceInvokables;
}
