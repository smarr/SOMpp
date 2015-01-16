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
#include "VMArray.h"
#include "VMSymbol.h"
//#include "VMClass.h"

#include "../misc/defs.h"

#if defined(_MSC_VER)   //Visual Studio
#include <windows.h> 
#include "../primitives/Core.h"
#endif

//class ClassGenerationContext;
//class VMSymbol;
//class VMArray;
class VMPrimitive;
class VMInvokable;

class VMClass: public VMObject {
public:
    typedef GCClass Stored;
    
    VMClass();
    VMClass(long numberOfFields);

    inline VMClass*     GetSuperClass();
    inline void         SetSuperClass(VMClass*);
    inline bool         HasSuperClass();
    inline VMSymbol*    GetName();
    inline void         SetName(VMSymbol*);
    inline VMArray*     GetInstanceFields();
    inline void         SetInstanceFields(VMArray*);
    inline VMArray*     GetInstanceInvokables();
           void         SetInstanceInvokables(VMArray*);
           long         GetNumberOfInstanceInvokables();
           VMInvokable* GetInstanceInvokable(long);
           void         SetInstanceInvokable(long, VMObject*);
           VMInvokable* LookupInvokable(VMSymbol*);
           long         LookupFieldIndex(VMSymbol*);
           bool         AddInstanceInvokable(VMObject*);
           void         AddInstancePrimitive(VMPrimitive*);
           VMSymbol*    GetInstanceFieldName(long);
           long         GetNumberOfInstanceFields();
           bool         HasPrimitives();
           void         LoadPrimitives();
    
#if GC_TYPE==PAUSELESS
    virtual VMClass*    Clone(Interpreter*);
    virtual VMClass*    Clone(PauselessCollectorThread*);
    virtual void MarkReferences();
    virtual void CheckMarking(void (vm_oop_t));
#else
    virtual VMClass*    Clone();
    void         WalkObjects(VMOBJECT_PTR (*walk)(VMOBJECT_PTR));
#endif
    
    virtual void MarkObjectAsInvalid();

private:
    void setPrimitives(const StdString& cname);
    long numberOfSuperInstanceFields();

    GCClass*  superClass;
    GCSymbol* name;
    GCArray*  instanceFields;
    GCArray*  instanceInvokables;

    static const long VMClassNumberOfFields;
};

VMClass* VMClass::GetSuperClass() {
    return READBARRIER(superClass);
}

void VMClass::SetSuperClass(VMClass* sup) {
    superClass = WRITEBARRIER(sup);
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, sup);
#endif
}

VMSymbol* VMClass::GetName() {
    return READBARRIER(name);
}

void VMClass::SetName(VMSymbol* nam) {
    name = WRITEBARRIER(nam);
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, nam);
#endif
}

bool VMClass::HasSuperClass() {
    VMClass* sc = READBARRIER(superClass);
    assert(Universe::IsValidObject(sc));

    // TODO: only for debugging, REMOVE!
    if (sc != READBARRIER(nilObject)) {
        assert((void*) sc != (void*) nilObject);
    }
    
    return sc != READBARRIER(nilObject);
}

VMArray* VMClass::GetInstanceFields() {
    return READBARRIER(instanceFields);
}

void VMClass::SetInstanceFields(VMArray* instFields) {
    instanceFields = WRITEBARRIER(instFields);
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, instFields);
#endif
}

VMArray* VMClass::GetInstanceInvokables() {
    return READBARRIER(instanceInvokables);
}
