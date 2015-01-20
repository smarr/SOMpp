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
#include "VMArray.h"
#include "VMSymbol.h"
#include "VMInvokable.h"
#include "VMPrimitive.h"
#include "PrimitiveRoutine.h"
#include <interpreter/Interpreter.h>

#include <primitivesCore/PrimitiveLoader.h>


const long VMClass::VMClassNumberOfFields = 4;

VMClass::VMClass() :
        VMObject(VMClassNumberOfFields), superClass(nullptr), name(nullptr), instanceFields(
                nullptr), instanceInvokables(nullptr) {
}

VMClass* VMClass::Clone(Page* page) {
    VMClass* clone = new (page, objectSize - sizeof(VMClass) ALLOC_MATURE) VMClass(*this);
    memcpy(SHIFTED_PTR(clone,sizeof(VMObject)),
           SHIFTED_PTR(this,sizeof(VMObject)),
           GetObjectSize() - sizeof(VMObject));
    return clone;
}

VMClass::VMClass(long numberOfFields) :
        VMObject(numberOfFields + VMClassNumberOfFields) {
}

void VMClass::MarkObjectAsInvalid() {
    VMObject::MarkObjectAsInvalid();
    superClass         = (GCClass*)  INVALID_GC_POINTER;
    name               = (GCSymbol*) INVALID_GC_POINTER;
    instanceFields     = (GCArray*)  INVALID_GC_POINTER;
    instanceInvokables = (GCArray*)  INVALID_GC_POINTER;
}

bool VMClass::addInstanceInvokable(VMInvokable* ptr, Page* page) {
    if (ptr == nullptr) {
        GetUniverse()->ErrorExit("Error: trying to add non-invokable to invokables array");
        return false;
    }
    //Check whether an invokable with the same signature exists and replace it if that's the case
    VMArray* instInvokables = GetInstanceInvokables();
    long numIndexableFields = instInvokables->GetNumberOfIndexableFields();
    for (long i = 0; i < numIndexableFields; ++i) {
        VMInvokable* inv = static_cast<VMInvokable*>(instInvokables->GetIndexableField(i));
        if (inv != nullptr) {
            if (ptr->GetSignature() == inv->GetSignature()) {
                SetInstanceInvokable(i, ptr);
                return false;
            }
        } else {
            GetUniverse()->ErrorExit("Invokables array corrupted. "
                                     "Either nullptr pointer added or pointer to non-invokable.");
            return false;
        }
    }
    //it's a new invokable so we need to expand the invokables array.
    store_ptr(instanceInvokables, instInvokables->CopyAndExtendWith(ptr, page));

    return true;
}

void VMClass::AddInstancePrimitive(VMPrimitive* ptr, Page* page) {
    if (addInstanceInvokable(ptr, page)) {
        //cout << "Warn: Primitive "<<ptr->GetSignature<<" is not in class definition for class " << name->GetStdString() << endl;
    }
}

VMSymbol* VMClass::GetInstanceFieldName(long index) {
    long numSuperInstanceFields = numberOfSuperInstanceFields();
    if (index >= numSuperInstanceFields) {
        index -= numSuperInstanceFields;
        return static_cast<VMSymbol*>(GetInstanceFields()->GetIndexableField(index));
    }
    return GetSuperClass()->GetInstanceFieldName(index);
}

void VMClass::SetInstanceInvokables(VMArray* invokables) {
    store_ptr(instanceInvokables, invokables);

    long numInvokables = GetNumberOfInstanceInvokables();
    for (long i = 0; i < numInvokables; ++i) {
        vm_oop_t invo = load_ptr(instanceInvokables)->GetIndexableField(i);
        //check for Nil object
        if (invo != load_ptr(nilObject)) {
            //not Nil, so this actually is an invokable
            VMInvokable* inv = static_cast<VMInvokable*>(invo);
            inv->SetHolder(this);
        }
    }
}

long VMClass::GetNumberOfInstanceInvokables() {
    return GetInstanceInvokables()->GetNumberOfIndexableFields();
}

VMInvokable* VMClass::GetInstanceInvokable(long index) {
    return static_cast<VMInvokable*>(GetInstanceInvokables()->GetIndexableField(index));
}

void VMClass::SetInstanceInvokable(long index, VMInvokable* invokable) {
    GetInstanceInvokables()->SetIndexableField(index, invokable);
    if (invokable != load_ptr(nilObject)) {
        invokable->SetHolder(this);
    }
}

VMInvokable* VMClass::LookupInvokable(VMSymbol* name) {
    assert(Universe::IsValidObject(this));
    
    /*
    VMInvokable* invokable = name->GetCachedInvokable(this);
    if (invokable != nullptr)
        return invokable; */

    long numInvokables = GetNumberOfInstanceInvokables();
    for (long i = 0; i < numInvokables; ++i) {
        VMInvokable* invokable = GetInstanceInvokable(i);
        
        VMSymbol* sig = invokable->GetSignature();
        if (sig == name) {
        //if (invokable->GetSignature() == name) {
//            name->UpdateCachedInvokable(this, invokable);
            return invokable;
        } else {
            if (strcmp(sig->GetChars(), name->GetChars()) == 0) {
                VMSymbol* sigTest = invokable->GetSignature();
                assert(0 != strcmp(sig->GetChars(), name->GetChars()));
            }
        }
    }

    // look in super class
    if (HasSuperClass()) {
        return GetSuperClass()->LookupInvokable(name);
    }
    
    // invokable not found
    return nullptr;
}

long VMClass::LookupFieldIndex(VMSymbol* name) {
    long numInstanceFields = GetNumberOfInstanceFields();
    for (long i = 0; i <= numInstanceFields; ++i) {
        // even with GetNumberOfInstanceFields == 0 there is the class field
        if (name == GetInstanceFieldName(i)) {
            return i;
        }
    }
    return -1;
}

long VMClass::GetNumberOfInstanceFields() {
    return GetInstanceFields()->GetNumberOfIndexableFields()
            + numberOfSuperInstanceFields();
}

bool VMClass::HasPrimitives() {
    long numInvokables = GetNumberOfInstanceInvokables();
    for (long i = 0; i < numInvokables; ++i) {
        VMInvokable* invokable = GetInstanceInvokable(i);
        if (invokable->IsPrimitive())
            return true;
    }
    return false;
}

void VMClass::LoadPrimitives(const vector<StdString>& cp, Page* page) {
    StdString cname = GetName()->GetStdString();
    
    if (hasPrimitivesFor(cname)) {
        setPrimitives(cname, false, page);
        GetClass()->setPrimitives(cname, true, page);
    }
}

long VMClass::numberOfSuperInstanceFields() {
    if (HasSuperClass())
        return GetSuperClass()->GetNumberOfInstanceFields();
    return 0;
}

bool VMClass::hasPrimitivesFor(const StdString& cl) const {
    return PrimitiveLoader::SupportsClass(cl);
}

/*
 * set the routines for primitive marked invokables of the given class
 */
void VMClass::setPrimitives(const StdString& cname, bool classSide, Page* page) {
    VMClass* current = this;

    // Try loading class-specific primitives for all super class' methods as well.
    while (current != load_ptr(nilObject)) {
        // iterate invokables
        long numInvokables = current->GetNumberOfInstanceInvokables();
        for (long i = 0; i < numInvokables; i++) {
            VMInvokable* anInvokable = current->GetInstanceInvokable(i);
#ifdef __DEBUG
            Universe::ErrorPrint("cname: >" + cname + "<\n" +
                                 anInvokable->GetSignature()->GetStdString() + "\n");
#endif

            VMSymbol* sig = anInvokable->GetSignature();
            StdString selector = sig->GetPlainString();

            PrimitiveRoutine* routine = PrimitiveLoader::GetPrimitiveRoutine(
                cname, selector, anInvokable->IsPrimitive() && current == this);
            
            if (routine && classSide == routine->isClassSide()) {
                VMPrimitive* thePrimitive;
                if (this == current && anInvokable->IsPrimitive()) {
                    thePrimitive = static_cast<VMPrimitive*>(anInvokable);
                } else {
                    thePrimitive = VMPrimitive::GetEmptyPrimitive(sig, classSide, page);
                    AddInstancePrimitive(thePrimitive, page);
                }

                // set routine
                thePrimitive->SetRoutine(routine);
                thePrimitive->SetEmpty(false);
            } else {
                if (anInvokable->IsPrimitive() && current == this) {
                    if (!routine || routine->isClassSide() == classSide) {
                        Universe::ErrorPrint("could not load primitive '" +
                                             selector + "' for class " +
                                             cname + "\n");
                        GetUniverse()->Quit(ERR_FAIL);
                    }
                }
            }
        }
        current = current->GetSuperClass();
    }
}

#if GC_TYPE==PAUSELESS
void VMClass::MarkReferences() {
    ReadBarrierForGCThread(&clazz);
    ReadBarrierForGCThread(&superClass);
    ReadBarrierForGCThread(&name);
    ReadBarrierForGCThread(&instanceFields);
    ReadBarrierForGCThread(&instanceInvokables);
    gc_oop_t* fields = FIELDS;
    for (long i = VMClassNumberOfFields + 0/*VMObjectNumberOfFields*/; i < numberOfFields; i++) {
        ReadBarrierForGCThread(&fields[i]);
    }
}
void VMClass::CheckMarking(void (*walk)(vm_oop_t)) {
    assert(GetNMTValue(clazz) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(clazz));
    walk(Untag(clazz));
    if (superClass) {
        assert(GetNMTValue(superClass) == _HEAP->GetGCThread()->GetExpectedNMT());
        CheckBlocked(Untag(superClass));
        walk(Untag(superClass));
        
    }
    assert(GetNMTValue(name) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(name));
    walk(Untag(name));
    assert(GetNMTValue(instanceFields) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(instanceFields));
    walk(Untag(instanceFields));
    assert(GetNMTValue(instanceInvokables) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(instanceInvokables));
    walk(Untag(instanceInvokables));
    for (long i = VMClassNumberOfFields + 0/*VMObjectNumberOfFields*/; i < numberOfFields; i++) {
        assert(GetNMTValue(FIELDS[i]) == _HEAP->GetGCThread()->GetExpectedNMT());
        CheckBlocked(Untag(FIELDS[i]));
        walk(Untag(FIELDS[i]));
    }
}
#else
void VMClass::WalkObjects(walk_heap_fn walk) {
    clazz = (GCClass*) (walk(clazz));
    if (superClass)
        superClass = (GCClass*) (walk(superClass));
    name = (GCSymbol*) (walk(name));
    instanceFields = (GCArray*) (walk(instanceFields));
    instanceInvokables = (GCArray*) (walk(instanceInvokables));
    
    //VMObject** fields = FIELDS;
    
    for (long i = VMClassNumberOfFields + 0/*VMObjectNumberOfFields*/; i < numberOfFields; i++)
        FIELDS[i] = (GCAbstractObject*) walk(FIELDS[i]);
}
#endif

StdString VMClass::AsDebugString() {
    return "Class(" + GetName()->GetStdString() + ")";
}
