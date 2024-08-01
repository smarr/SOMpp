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

#include "../misc/defs.h"
#include "../vm/Globals.h"
#include "../vm/IsValidObject.h"
#include "VMObject.h"

#if defined(_MSC_VER)  // Visual Studio
  #include <windows.h>

  #include "../primitives/Core.h"
#endif

class ClassGenerationContext;

class VMClass : public VMObject {
public:
    typedef GCClass Stored;

    VMClass();
    VMClass(size_t numberOfFields, size_t additionalBytes);

    VMObject* GetSuperClass() const;
    void SetSuperClass(VMObject*);
    bool HasSuperClass() const;
    VMSymbol* GetName() const;
    void SetName(VMSymbol*);
    VMArray* GetInstanceFields() const;
    void SetInstanceFields(VMArray*);
    VMArray* GetInstanceInvokables() const;
    void SetInstanceInvokables(VMArray*);
    size_t GetNumberOfInstanceInvokables() const;
    VMInvokable* GetInstanceInvokable(long) const;
    void SetInstanceInvokable(long, VMInvokable*);
    VMInvokable* LookupInvokable(VMSymbol*) const;
    long LookupFieldIndex(VMSymbol*) const;
    bool AddInstanceInvokable(VMInvokable*);
    void AddInstancePrimitive(VMPrimitive*);
    VMSymbol* GetInstanceFieldName(long) const;
    size_t GetNumberOfInstanceFields() const;
    bool HasPrimitives() const;
    void LoadPrimitives();
    VMClass* CloneForMovingGC() const override;

    StdString AsDebugString() const override;

private:
    bool hasPrimitivesFor(const StdString& cl) const;
    void setPrimitives(const StdString& cname, bool classSide);
    size_t numberOfSuperInstanceFields() const;

    static const size_t VMClassNumberOfFields;

    make_testable(public);

    // Remember to update Parser::superclass when the fields are changed
    GCSymbol* name;
    GCArray* instanceFields;
    GCArray* instanceInvokables;
    GCObject* superClass;
};
