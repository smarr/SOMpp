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
pVMClass VMClass::Clone() {
    pVMClass clone = new (_HEAP, _PAGE, objectSize - sizeof(VMClass), true)VMClass(*this);
    memcpy(SHIFTED_PTR(clone,sizeof(VMObject)), SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    return clone;
}
#elif GC_TYPE==PAUSELESS
pVMClass VMClass::Clone(Interpreter* thread) {
    pVMClass clone = new (_HEAP, thread, objectSize - sizeof(VMClass)) VMClass(*this);
    memcpy(SHIFTED_PTR(clone,sizeof(VMObject)), SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    clone->IncreaseVersion();
    /* this->MarkObjectAsInvalid(); */
    return clone;
}
pVMClass VMClass::Clone(PauselessCollectorThread* thread) {
    pVMClass clone = new (_HEAP, thread, objectSize - sizeof(VMClass)) VMClass(*this);
    memcpy(SHIFTED_PTR(clone,sizeof(VMObject)), SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    clone->IncreaseVersion();
    /* this->MarkObjectAsInvalid(); */
    return clone;
}
#else
pVMClass VMClass::Clone() {
    pVMClass clone = new (_HEAP, objectSize - sizeof(VMClass))VMClass(*this);
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

bool VMClass::AddInstanceInvokable(pVMObject ptr) {
    pVMInvokable newInvokable = static_cast<pVMInvokable>(ptr);
    if (newInvokable == NULL) {
        _UNIVERSE->ErrorExit("Error: trying to add non-invokable to invokables array");
    }
    //Check whether an invokable with the same signature exists and replace it if that's the case
    long numIndexableFields = this->GetInstanceInvokables()->GetNumberOfIndexableFields();
    for (long i = 0; i < numIndexableFields; ++i) {
        pVMInvokable inv = static_cast<pVMInvokable>(this->GetInstanceInvokables()->GetIndexableField(i));
        if (inv != NULL) {
            if (newInvokable->GetSignature() == inv->GetSignature()) {
                this->SetInstanceInvokable(i, ptr);
                return false;
            }
        } else {
            _UNIVERSE->ErrorExit("Invokables array corrupted. Either NULL pointer added or pointer to non-invokable.");
        }
    }
    //it's a new invokable so we need to expand the invokables array.
    instanceInvokables = WRITEBARRIER(this->GetInstanceInvokables()->CopyAndExtendWith(ptr));
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, instanceInvokables);
#endif

    return true;
}

void VMClass::AddInstancePrimitive(pVMPrimitive ptr) {
    if (AddInstanceInvokable(ptr)) {
        //cout << "Warn: Primitive "<<ptr->GetSignature<<" is not in class definition for class " << name->GetStdString() << endl;
    }
}

pVMSymbol VMClass::GetInstanceFieldName(long index) {
    long numSuperInstanceFields = numberOfSuperInstanceFields();
    if (index >= numSuperInstanceFields) {
        index -= numSuperInstanceFields;
        return static_cast<pVMSymbol>(this->GetInstanceFields()->GetIndexableField(index));
    }
    return this->GetSuperClass()->GetInstanceFieldName(index);
}

void VMClass::SetInstanceInvokables(pVMArray invokables) {
    instanceInvokables = WRITEBARRIER(invokables);
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, instanceInvokables);
#endif

    long numInvokables = GetNumberOfInstanceInvokables();
    for (long i = 0; i < numInvokables; ++i) {
        pVMObject invo = READBARRIER(instanceInvokables)->GetIndexableField(i);
        //check for Nil object
        if (invo != READBARRIER(nilObject)) {
            //not Nil, so this actually is an invokable
            pVMInvokable inv = static_cast<pVMInvokable>(invo);
            inv->SetHolder(this);
        }
    }
}

long VMClass::GetNumberOfInstanceInvokables() {
    return this->GetInstanceInvokables()->GetNumberOfIndexableFields();
}

pVMInvokable VMClass::GetInstanceInvokable(long index) {
    return static_cast<pVMInvokable>(this->GetInstanceInvokables()->GetIndexableField(index));
}

void VMClass::SetInstanceInvokable(long index, pVMObject invokable) {
    this->GetInstanceInvokables()->SetIndexableField(index, invokable);
    if (invokable != READBARRIER(nilObject)) {
        pVMInvokable inv = static_cast<pVMInvokable>(invokable);
        inv->SetHolder(this);
    }
}

pVMInvokable VMClass::LookupInvokable(pVMSymbol name) {
    assert(Universe::IsValidObject(this));
    
    /*
    pVMInvokable invokable = name->GetCachedInvokable(this);
    if (invokable != NULL)
        return invokable; */
    pVMInvokable invokable;

    long numInvokables = GetNumberOfInstanceInvokables();
    for (long i = 0; i < numInvokables; ++i) {
        invokable = GetInstanceInvokable(i);
        
        pVMSymbol sig = invokable->GetSignature();
        if (sig == name) {
        //if (invokable->GetSignature() == name) {
            //name->UpdateCachedInvokable(this, invokable);
            return invokable;
        } else {
            if (strcmp(sig->GetChars(), name->GetChars()) == 0) {
                pVMSymbol sigTest = invokable->GetSignature();
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

long VMClass::LookupFieldIndex(pVMSymbol name) {
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
        pVMInvokable invokable = GetInstanceInvokable(i);
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
    pVMPrimitive thePrimitive;
    PrimitiveRoutine* routine = NULL;
    pVMInvokable anInvokable;
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
            thePrimitive = static_cast<pVMPrimitive>(anInvokable);
            //
            // we have a primitive to load
            // get it's selector
            //
            pVMSymbol sig = thePrimitive->GetSignature();
            StdString selector = sig->GetPlainString();
            routine = get_primitive(cname, selector);

            if(!routine) {
                cout << "could not load primitive '"<< selector
                <<"' for class " << cname << endl;

                _UNIVERSE->Quit(ERR_FAIL);
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
void VMClass::CheckMarking(void (*walk)(AbstractVMObject*)) {
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
    clazz = static_cast<pVMClass>(walk(clazz));
    if (superClass)
        superClass = static_cast<pVMClass>(walk(superClass));
    name = static_cast<pVMSymbol>(walk(name));
    instanceFields = static_cast<pVMArray>(walk(instanceFields));
    instanceInvokables = static_cast<pVMArray>(walk(instanceInvokables));
    
    pVMObject* fields = FIELDS;
    
    for (long i = VMClassNumberOfFields + 0/*VMObjectNumberOfFields*/; i < numberOfFields; i++)
        fields[i] = walk(AS_POINTER(fields[i]));
}
#endif
