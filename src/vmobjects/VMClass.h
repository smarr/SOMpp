#pragma once
#ifndef VMCLASS_H_
#define VMCLASS_H_

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

    inline pVMClass     GetSuperClass();
    inline void         SetSuperClass(pVMClass);
    inline bool         HasSuperClass();
    inline pVMSymbol    GetName();
    inline void         SetName(pVMSymbol);
    inline pVMArray     GetInstanceFields();
    inline void         SetInstanceFields(pVMArray);
    inline pVMArray     GetInstanceInvokables();
           void         SetInstanceInvokables(pVMArray);
           long         GetNumberOfInstanceInvokables();
           pVMInvokable GetInstanceInvokable(long);
           void         SetInstanceInvokable(long, pVMObject);
           pVMInvokable LookupInvokable(pVMSymbol);
           long         LookupFieldIndex(pVMSymbol);
           bool         AddInstanceInvokable(pVMObject);
           void         AddInstancePrimitive(pVMPrimitive);
           pVMSymbol    GetInstanceFieldName(long);
           long         GetNumberOfInstanceFields();
           bool         HasPrimitives();
           void         LoadPrimitives();
    
#if GC_TYPE==PAUSELESS
    virtual pVMClass    Clone(Interpreter*);
    virtual pVMClass    Clone(PauselessCollectorThread*);
    virtual void MarkReferences();
    virtual void CheckMarking(void (AbstractVMObject*));
#else
    virtual pVMClass    Clone();
    void         WalkObjects(VMOBJECT_PTR (*walk)(VMOBJECT_PTR));
#endif
    
    virtual void MarkObjectAsInvalid();

private:
    void setPrimitives(const StdString& cname);
    long numberOfSuperInstanceFields();

    pVMClass superClass;
    pVMSymbol name;
    pVMArray instanceFields;
    pVMArray instanceInvokables;

    static const long VMClassNumberOfFields;
};

pVMClass VMClass::GetSuperClass() {
    return READBARRIER(superClass);
}

void VMClass::SetSuperClass(pVMClass sup) {
    superClass = WRITEBARRIER(sup);
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, sup);
#endif
}

pVMSymbol VMClass::GetName() {
    return READBARRIER(name);
}

void VMClass::SetName(pVMSymbol nam) {
    name = WRITEBARRIER(nam);
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, nam);
#endif
}

bool VMClass::HasSuperClass() {
    pVMClass sc = READBARRIER(superClass);
    assert(Universe::IsValidObject(sc));
    if (sc != READBARRIER(nilObject)) {
        assert(sc != nilObject);
    }
    return sc != READBARRIER(nilObject);
}

pVMArray VMClass::GetInstanceFields() {
    return READBARRIER(instanceFields);
}

void VMClass::SetInstanceFields(pVMArray instFields) {
    instanceFields = WRITEBARRIER(instFields);
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, instFields);
#endif
}

pVMArray VMClass::GetInstanceInvokables() {
    return READBARRIER(instanceInvokables);
}

#endif
