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

#include "VMClass.h"

#include <cassert>
#include <cstring>
#include <string>

#include "../memory/Heap.h"
#include "../misc/defs.h"
#include "../primitivesCore/PrimitiveLoader.h"
#include "../vm/Globals.h"
#include "../vm/IsValidObject.h"
#include "../vm/Print.h"
#include "../vm/Universe.h"
#include "ObjectFormats.h"
#include "PrimitiveRoutine.h"
#include "VMArray.h"
#include "VMInvokable.h"
#include "VMObject.h"
#include "VMPrimitive.h"
#include "VMSymbol.h"

const size_t VMClass::VMClassNumberOfFields = 4;

VMClass::VMClass()
    : VMObject(VMClassNumberOfFields, sizeof(VMClass)), name(nullptr),
      instanceFields(nullptr), instanceInvokables(nullptr),
      superClass(nullptr) {}

VMClass* VMClass::CloneForMovingGC() const {
    VMClass* clone =
        new (GetHeap<HEAP_CLS>(),
             totalObjectSize - sizeof(VMClass) ALLOC_MATURE) VMClass(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)),
           SHIFTED_PTR(this, sizeof(VMObject)),
           GetObjectSize() - sizeof(VMObject));
    return clone;
}

VMClass::VMClass(size_t numberOfFields, size_t additionalBytes)
    : VMObject(numberOfFields + VMClassNumberOfFields,
               additionalBytes + sizeof(VMClass)) {}

bool VMClass::AddInstanceInvokable(VMInvokable* ptr) {
    if (ptr == nullptr) {
        GetUniverse()->ErrorExit(
            "Error: trying to add non-invokable to invokables array");
        return false;
    }
    // Check whether an invokable with the same signature exists and replace it
    // if that's the case
    VMArray* instInvokables = load_ptr(instanceInvokables);
    size_t numIndexableFields = instInvokables->GetNumberOfIndexableFields();
    for (size_t i = 0; i < numIndexableFields; ++i) {
        VMInvokable* inv =
            static_cast<VMInvokable*>(instInvokables->GetIndexableField(i));
        if (inv != nullptr) {
            if (ptr->GetSignature() == inv->GetSignature()) {
                SetInstanceInvokable(i, ptr);
                return false;
            }
        } else {
            GetUniverse()->ErrorExit(
                "Invokables array corrupted. "
                "Either NULL pointer added or pointer to non-invokable.");
            return false;
        }
    }
    // it's a new invokable so we need to expand the invokables array.
    store_ptr(instanceInvokables,
              instInvokables->CopyAndExtendWith((vm_oop_t)ptr));

    return true;
}

void VMClass::AddInstancePrimitive(VMPrimitive* ptr) {
    if (AddInstanceInvokable(ptr)) {
        // cout << "Warn: Primitive "<<ptr->GetSignature<<" is not in class
        // definition for class " << name->GetStdString() << endl;
    }
}

VMSymbol* VMClass::GetInstanceFieldName(long index) const {
    long numSuperInstanceFields = numberOfSuperInstanceFields();
    if (index >= numSuperInstanceFields) {
        index -= numSuperInstanceFields;
        return static_cast<VMSymbol*>(
            load_ptr(instanceFields)->GetIndexableField(index));
    }
    assert(HasSuperClass());
    return ((VMClass*)load_ptr(superClass))->GetInstanceFieldName(index);
}

void VMClass::SetInstanceInvokables(VMArray* invokables) {
    store_ptr(instanceInvokables, invokables);
    vm_oop_t nil = load_ptr(nilObject);

    size_t numInvokables = GetNumberOfInstanceInvokables();
    for (size_t i = 0; i < numInvokables; ++i) {
        vm_oop_t invo = load_ptr(instanceInvokables)->GetIndexableField(i);
        // check for Nil object
        if (invo != nil) {
            // not Nil, so this actually is an invokable
            VMInvokable* inv = (VMInvokable*)invo;
            inv->SetHolder(this);
        }
    }
}

size_t VMClass::GetNumberOfInstanceInvokables() const {
    return load_ptr(instanceInvokables)->GetNumberOfIndexableFields();
}

VMInvokable* VMClass::GetInstanceInvokable(long index) const {
    return static_cast<VMInvokable*>(
        load_ptr(instanceInvokables)->GetIndexableField(index));
}

void VMClass::SetInstanceInvokable(long index, VMInvokable* invokable) {
    load_ptr(instanceInvokables)->SetIndexableField(index, invokable);
    if (invokable != reinterpret_cast<VMInvokable*>(load_ptr(nilObject))) {
        invokable->SetHolder(this);
    }
}

VMInvokable* VMClass::LookupInvokable(VMSymbol* name) const {
    assert(IsValidObject(const_cast<VMClass*>(this)));

    VMInvokable* invokable = name->GetCachedInvokable(this);
    if (invokable != nullptr) {
        return invokable;
    }

    long numInvokables = GetNumberOfInstanceInvokables();
    for (long i = 0; i < numInvokables; ++i) {
        invokable = GetInstanceInvokable(i);
        if (invokable->GetSignature() == name) {
            name->UpdateCachedInvokable(this, invokable);
            return invokable;
        }
    }

    // look in super class
    if (HasSuperClass()) {
        return ((VMClass*)load_ptr(superClass))->LookupInvokable(name);
    }

    // invokable not found
    return nullptr;
}

long VMClass::LookupFieldIndex(VMSymbol* name) const {
    long numInstanceFields = GetNumberOfInstanceFields();
    for (long i = 0; i <= numInstanceFields; ++i) {
        // even with GetNumberOfInstanceFields == 0 there is the class field
        if (name == GetInstanceFieldName(i)) {
            return i;
        }
    }
    return -1;
}

size_t VMClass::GetNumberOfInstanceFields() const {
    return load_ptr(instanceFields)->GetNumberOfIndexableFields() +
           numberOfSuperInstanceFields();
}

bool VMClass::HasPrimitives() const {
    long numInvokables = GetNumberOfInstanceInvokables();
    for (long i = 0; i < numInvokables; ++i) {
        VMInvokable* invokable = GetInstanceInvokable(i);
        if (invokable->IsPrimitive()) {
            return true;
        }
    }
    return false;
}

void VMClass::LoadPrimitives() {
    std::string cname = load_ptr(name)->GetStdString();

    if (hasPrimitivesFor(cname)) {
        setPrimitives(cname, false);
        GetClass()->setPrimitives(cname, true);
    }
}

size_t VMClass::numberOfSuperInstanceFields() const {
    if (HasSuperClass()) {
        return ((VMClass*)load_ptr(superClass))->GetNumberOfInstanceFields();
    }
    return 0;
}

bool VMClass::hasPrimitivesFor(const std::string& cl) const {
    return PrimitiveLoader::SupportsClass(cl);
}

/*
 * set the routines for primitive marked invokables of the given class
 */
void VMClass::setPrimitives(const std::string& cname, bool classSide) {
    VMObject* current = this;

    // Try loading class-specific primitives for all super class' methods as
    // well.
    while (current != load_ptr(nilObject)) {
        VMClass* currentClass = (VMClass*)current;

        // iterate invokables
        long numInvokables = currentClass->GetNumberOfInstanceInvokables();
        for (long i = 0; i < numInvokables; i++) {
            VMInvokable* anInvokable = currentClass->GetInstanceInvokable(i);
#ifdef __DEBUG
            ErrorPrint("cname: >" + cname + "<\n" +
                       anInvokable->GetSignature()->GetStdString() + "\n");
#endif

            VMSymbol* sig = anInvokable->GetSignature();
            std::string selector = sig->GetPlainString();

            PrimitiveRoutine* routine = PrimitiveLoader::GetPrimitiveRoutine(
                cname, selector, anInvokable->IsPrimitive() && current == this);

            if (routine && classSide == routine->isClassSide()) {
                VMPrimitive* thePrimitive;
                if (this == current && anInvokable->IsPrimitive()) {
                    thePrimitive = static_cast<VMPrimitive*>(anInvokable);
                } else {
                    thePrimitive =
                        VMPrimitive::GetEmptyPrimitive(sig, classSide);
                    AddInstancePrimitive(thePrimitive);
                }

                // set routine
                thePrimitive->SetRoutine(routine, false);
            } else {
                if (anInvokable->IsPrimitive() && current == this) {
                    if (!routine || routine->isClassSide() == classSide) {
                        ErrorPrint("could not load primitive '" + selector +
                                   "' for class " + cname + "\n");
                        GetUniverse()->Quit(ERR_FAIL);
                    }
                }
            }
        }
        current = currentClass->GetSuperClass();
    }
}

std::string VMClass::AsDebugString() const {
    return "Class(" + GetName()->GetStdString() + ")";
}

VMObject* VMClass::GetSuperClass() const {
    return load_ptr(superClass);
}

void VMClass::SetSuperClass(VMObject* sup) {
    store_ptr(superClass, sup);
}

VMSymbol* VMClass::GetName() const {
    return load_ptr(name);
}

void VMClass::SetName(VMSymbol* nam) {
    store_ptr(name, nam);
}

bool VMClass::HasSuperClass() const {
    assert(IsValidObject(load_ptr(superClass)));
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
