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
#include "../interpreter/Interpreter.h"
#include <primitives/Core.h>

#include <fstream>
#include <typeinfo>

/*
 * Format definitions for Primitive naming scheme.
 *
 */
#define CLASS_METHOD_FORMAT_S "%s::%s"
// as in AClass::aClassMethod
#define INSTANCE_METHOD_FORMAT_S "%s::%s_"
// as in AClass::anInstanceMethod_

const long VMClass::VMClassNumberOfFields = 4;

VMClass::VMClass() :
        VMObject(VMClassNumberOfFields), superClass(NULL), name(NULL), instanceFields(
                NULL), instanceInvokables(NULL) {
}

#if GC_TYPE==GENERATIONAL
VMClass* VMClass::Clone() {
    VMClass* clone = new (_HEAP, _PAGE, objectSize - sizeof(VMClass), true)VMClass(*this);
    memcpy(SHIFTED_PTR(clone,sizeof(VMObject)), SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    return clone;
}
#elif GC_TYPE==PAUSELESS
VMClass* VMClass::Clone(Interpreter* thread) {
    VMClass* clone = new (_HEAP, thread, objectSize - sizeof(VMClass)) VMClass(*this);
    memcpy(SHIFTED_PTR(clone,sizeof(VMObject)), SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    /* clone->IncreaseVersion();
    this->MarkObjectAsInvalid(); */
    return clone;
}
VMClass* VMClass::Clone(PauselessCollectorThread* thread) {
    VMClass* clone = new (_HEAP, thread, objectSize - sizeof(VMClass)) VMClass(*this);
    memcpy(SHIFTED_PTR(clone,sizeof(VMObject)), SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    /* clone->IncreaseVersion();
    this->MarkObjectAsInvalid(); */
    return clone;
}
#else
VMClass* VMClass::Clone() {
    VMClass* clone = new (_HEAP, objectSize - sizeof(VMClass))VMClass(*this);
    memcpy(SHIFTED_PTR(clone,sizeof(VMObject)), SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    return clone;
}
#endif

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

bool VMClass::AddInstanceInvokable(VMObject* ptr) {
    VMInvokable* newInvokable = static_cast<VMInvokable*>(ptr);
    if (newInvokable == NULL) {
        GetUniverse()->ErrorExit("Error: trying to add non-invokable to invokables array");
    }
    //Check whether an invokable with the same signature exists and replace it if that's the case
    long numIndexableFields = this->GetInstanceInvokables()->GetNumberOfIndexableFields();
    for (long i = 0; i < numIndexableFields; ++i) {
        VMInvokable* inv = static_cast<VMInvokable*>(this->GetInstanceInvokables()->GetIndexableField(i));
        if (inv != NULL) {
            if (newInvokable->GetSignature() == inv->GetSignature()) {
                this->SetInstanceInvokable(i, ptr);
                return false;
            }
        } else {
            GetUniverse()->ErrorExit("Invokables array corrupted. Either NULL pointer added or pointer to non-invokable.");
        }
    }
    //it's a new invokable so we need to expand the invokables array.
    instanceInvokables = WRITEBARRIER(this->GetInstanceInvokables()->CopyAndExtendWith(ptr));
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, READBARRIER(instanceInvokables));
#endif

    return true;
}

void VMClass::AddInstancePrimitive(VMPrimitive* ptr) {
    if (AddInstanceInvokable(ptr)) {
        //cout << "Warn: Primitive "<<ptr->GetSignature<<" is not in class definition for class " << name->GetStdString() << endl;
    }
}

VMSymbol* VMClass::GetInstanceFieldName(long index) {
    long numSuperInstanceFields = numberOfSuperInstanceFields();
    if (index >= numSuperInstanceFields) {
        index -= numSuperInstanceFields;
        return static_cast<VMSymbol*>(this->GetInstanceFields()->GetIndexableField(index));
    }
    return this->GetSuperClass()->GetInstanceFieldName(index);
}

void VMClass::SetInstanceInvokables(VMArray* invokables) {
    instanceInvokables = WRITEBARRIER(invokables);
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, READBARRIER(instanceInvokables));
#endif

    long numInvokables = GetNumberOfInstanceInvokables();
    for (long i = 0; i < numInvokables; ++i) {
        VMObject* invo = READBARRIER(instanceInvokables)->GetIndexableField(i);
        //check for Nil object
        if (invo != READBARRIER(nilObject)) {
            //not Nil, so this actually is an invokable
            VMInvokable* inv = static_cast<VMInvokable*>(invo);
            inv->SetHolder(this);
        }
    }
}

long VMClass::GetNumberOfInstanceInvokables() {
    return this->GetInstanceInvokables()->GetNumberOfIndexableFields();
}

VMInvokable* VMClass::GetInstanceInvokable(long index) {
    return static_cast<VMInvokable*>(this->GetInstanceInvokables()->GetIndexableField(index));
}

void VMClass::SetInstanceInvokable(long index, VMObject* invokable) {
    this->GetInstanceInvokables()->SetIndexableField(index, invokable);
    if (invokable != READBARRIER(nilObject)) {
        VMInvokable* inv = static_cast<VMInvokable*>(invokable);
        inv->SetHolder(this);
    }
}

VMInvokable* VMClass::LookupInvokable(VMSymbol* name) {
    assert(Universe::IsValidObject(this));
    
    /*
    VMInvokable* invokable = name->GetCachedInvokable(this);
    if (invokable != NULL)
        return invokable; */
    VMInvokable* invokable;

    long numInvokables = GetNumberOfInstanceInvokables();
    for (long i = 0; i < numInvokables; ++i) {
        invokable = GetInstanceInvokable(i);
        
        VMSymbol* sig = invokable->GetSignature();
        if (sig == name) {
        //if (invokable->GetSignature() == name) {
            //name->UpdateCachedInvokable(this, invokable);
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
        return this->GetSuperClass()->LookupInvokable(name);
    }
    
    // invokable not found
    return NULL;
}

long VMClass::LookupFieldIndex(VMSymbol* name) {
    long numInstanceFields = GetNumberOfInstanceFields();
    for (long i = 0; i <= numInstanceFields; ++i)
        // even with GetNumberOfInstanceFields == 0 there is the class field
        if (name == GetInstanceFieldName(i)) {
            return i;
        }
    return -1;
}

long VMClass::GetNumberOfInstanceFields() {
    return this->GetInstanceFields()->GetNumberOfIndexableFields()
            + this->numberOfSuperInstanceFields();
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

void VMClass::LoadPrimitives() {

    // cached object properties
    StdString cname = this->GetName()->GetStdString();

    setPrimitives(cname);
    this->GetClass()->setPrimitives(cname);
}

long VMClass::numberOfSuperInstanceFields() {
    if (this->HasSuperClass())
        return this->GetSuperClass()->GetNumberOfInstanceFields();
    return 0;
}

/*
 * set the routines for primitive marked invokables of the given class
 *
 */
void VMClass::setPrimitives(const StdString& cname) {
    VMPrimitive* thePrimitive;
    PrimitiveRoutine* routine = NULL;
    VMInvokable* anInvokable;
    // iterate invokables
    long numInvokables = GetNumberOfInstanceInvokables();
    for (long i = 0; i < numInvokables; i++) {
        anInvokable = this->GetInstanceInvokable(i);
#ifdef __DEBUG
        cout << "cname: >" << cname << "<"<< endl;
        cout << anInvokable->GetSignature()->GetStdString() << endl;
#endif
        if(anInvokable->IsPrimitive()) {
#ifdef __DEBUG
            cout << "... is a primitive, and is going to be loaded now" << endl;
#endif
            thePrimitive = static_cast<VMPrimitive*>(anInvokable);
            //
            // we have a primitive to load
            // get it's selector
            //
            VMSymbol* sig = thePrimitive->GetSignature();
            StdString selector = sig->GetPlainString();
            routine = get_primitive(cname, selector);

            if(!routine) {
                cout << "could not load primitive '"<< selector
                <<"' for class " << cname << endl;

                GetUniverse()->Quit(ERR_FAIL);
            }

            // set routine
            thePrimitive->SetRoutine(routine);
            thePrimitive->SetEmpty(false);
        }
#ifdef __DEBUG
        else {
            cout << "... is not a primitive" << endl;
        }
#endif
    }
}

#if GC_TYPE==PAUSELESS
void VMClass::MarkReferences() {
    ReadBarrierForGCThread(&clazz);
    ReadBarrierForGCThread(&superClass);
    ReadBarrierForGCThread(&name);
    ReadBarrierForGCThread(&instanceFields);
    ReadBarrierForGCThread(&instanceInvokables);
    GCAbstractObject** fields = FIELDS;
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
        assert(GetNMTValue(AS_GC_POINTER(FIELDS[i])) == _HEAP->GetGCThread()->GetExpectedNMT());
        CheckBlocked(Untag(AS_GC_POINTER(FIELDS[i])));
        walk(Untag(AS_GC_POINTER(FIELDS[i])));
    }
}
#else
void VMClass::WalkObjects(VMOBJECT_PTR (*walk)(VMOBJECT_PTR)) {
    clazz = (GCClass*) (walk(READBARRIER(clazz)));
    if (superClass)
        superClass = (GCClass*) (walk(READBARRIER(superClass)));
    name = (GCSymbol*) (walk(READBARRIER(name)));
    instanceFields = (GCArray*) (walk(READBARRIER(instanceFields)));
    instanceInvokables = (GCArray*) (walk(READBARRIER(instanceInvokables)));
    
    //VMObject** fields = FIELDS;
    
    for (long i = VMClassNumberOfFields + 0/*VMObjectNumberOfFields*/; i < numberOfFields; i++)
        FIELDS[i] = (GCAbstractObject*) walk(AS_VM_POINTER(FIELDS[i]));
}
#endif
