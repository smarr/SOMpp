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

#include <misc/defs.h>

#include <vmobjects/VMObject.h>
#include <vmobjects/VMArray.h>
#include <vmobjects/VMSymbol.h>


#if defined(_MSC_VER)   //Visual Studio
#include <windows.h> 
#include <primitives/Core.h>
#endif

class VMPrimitive;
class VMInvokable;

class VMClass: public VMObject {
public:
    typedef GCClass Stored;
    
    VMClass();
    VMClass(size_t numberOfGcPtrFields);

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
           void         SetInstanceInvokable(long, VMInvokable*);
           VMInvokable* LookupInvokable(VMSymbol*);
           long         LookupFieldIndex(VMSymbol*);
           void         AddInstancePrimitive(VMPrimitive*, Page*);
           VMSymbol*    GetInstanceFieldName(long);
           long         GetNumberOfInstanceFields();
           bool         HasPrimitives();
           void         LoadPrimitives(const vector<StdString>&, Page*);
    virtual VMClass*    Clone(Page*);

    virtual StdString AsDebugString();

private:
    bool addInstanceInvokable(VMInvokable*, Page*);
    bool hasPrimitivesFor(const StdString& cl) const;
    void setPrimitives(const StdString& cname, bool classSide, Page*);
    size_t numberOfSuperInstanceFields();

    // normal GC ptr fields, handled implicitly by VMObject
    GCClass*  superClass;
    GCSymbol* name;
    GCArray*  instanceFields;
    GCArray*  instanceInvokables;

    static const size_t VMClassNumberOfGcPtrFields;
};

VMClass* VMClass::GetSuperClass() {
    return load_ptr(superClass);
}

void VMClass::SetSuperClass(VMClass* sup) {
    store_ptr(superClass, sup);
}

VMSymbol* VMClass::GetName() {
    return load_ptr(name);
}

void VMClass::SetName(VMSymbol* nam) {
    store_ptr(name, nam);
}

bool VMClass::HasSuperClass() {
    assert(Universe::IsValidObject(load_ptr(superClass)));
    return load_ptr(superClass) != load_ptr(nilObject);
}

VMArray* VMClass::GetInstanceFields() {
    return load_ptr(instanceFields);
}

void VMClass::SetInstanceFields(VMArray* instFields) {
    store_ptr(instanceFields, instFields);
}

VMArray* VMClass::GetInstanceInvokables() {
    return load_ptr(instanceInvokables);
}
