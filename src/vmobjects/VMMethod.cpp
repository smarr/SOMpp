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

#include "VMBlock.h"
#include "VMMethod.h"
#include "VMFrame.h"
#include "VMClass.h"
#include "VMSymbol.h"
#include "VMArray.h"
#include "VMObject.h"
#include "VMInteger.h"
#include "Signature.h"
#include <interpreter/Interpreter.h>

#include <vm/Universe.h>
#include <vmobjects/VMBlock.inline.h>
#include <vmobjects/VMMethod.inline.h>

#ifdef UNSAFE_FRAME_OPTIMIZATION
const long VMMethod::VMMethodNumberOfFields = 8;
#else
const long VMMethod::VMMethodNumberOfFields = 7;
#endif

VMMethod::VMMethod(long bcCount, long numberOfConstants, long nof) :
        VMInvokable(nof + VMMethodNumberOfFields) {
#ifdef UNSAFE_FRAME_OPTIMIZATION
    cachedFrame = nullptr;
#endif
# warning not sure whether the use of _store_ptr is ok here
# warning, if we use extra parts of the heap for the allocation, we probably need to trigger the generational barrier
    bcLength                     = _store_ptr(NEW_INT(bcCount));
    numberOfLocals               = _store_ptr(NEW_INT(0));
    maximumNumberOfStackElements = _store_ptr(NEW_INT(0));
    numberOfArguments            = _store_ptr(NEW_INT(0));
    this->numberOfConstants      = _store_ptr(NEW_INT(numberOfConstants));

    indexableFields = (gc_oop_t*)(&indexableFields + 2);  // this is just a hack to get the convenience pointer, the fields start after the two other remaining fields in VMMethod
    for (long i = 0; i < numberOfConstants; ++i) {
#warning do we need to cylce through the barriers here?
        store_ptr(indexableFields[i], load_ptr(nilObject));
    }
    bytecodes = (uint8_t*)(&indexableFields + 2 + GetNumberOfIndexableFields());
}

#if GC_TYPE==GENERATIONAL
VMMethod* VMMethod::Clone() {
    VMMethod* clone = new (_HEAP, _PAGE, GetObjectSize() - sizeof(VMMethod), true)
    VMMethod(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this, sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    clone->indexableFields = (gc_oop_t*)(&(clone->indexableFields) + 2);
    clone->bytecodes = (uint8_t*)(&(clone->indexableFields) + 2 + GetNumberOfIndexableFields());
    return clone;
}
#elif GC_TYPE==PAUSELESS
VMMethod* VMMethod::Clone(Interpreter* thread) {
    VMMethod* clone = new (_HEAP, thread, GetObjectSize() - sizeof(VMMethod)) VMMethod(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this, sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    clone->indexableFields = (gc_oop_t*)(&(clone->indexableFields) + 2);  // this is just a hack to get the convenience pointer, the fields start after the two other remaining fields in VMMethod
    clone->bytecodes = (uint8_t*)(&(clone->indexableFields) + 2 + GetNumberOfIndexableFields());
    /* clone->IncreaseVersion(); */
    return clone;
}
VMMethod* VMMethod::Clone(PauselessCollectorThread* thread) {
    VMMethod* clone = new (_HEAP, thread, GetObjectSize() - sizeof(VMMethod)) VMMethod(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this, sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    clone->indexableFields = (gc_oop_t*)(&(clone->indexableFields) + 2);  // this is just a hack to get the convenience pointer, the fields start after the two other remaining fields in VMMethod
    clone->bytecodes = (uint8_t*)(&(clone->indexableFields) + 2 + INT_VAL(ReadBarrierForGCThread(&numberOfConstants)));
    /* clone->IncreaseVersion(); */
    return clone;
}
#else
VMMethod* VMMethod::Clone() {
    VMMethod* clone = new (_HEAP, GetObjectSize() - sizeof(VMMethod)) VMMethod(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this, sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    clone->indexableFields = (VMObject**)(&(clone->indexableFields) + 2);
    clone->bytecodes = (uint8_t*)(&(clone->indexableFields) + 2 + GetNumberOfIndexableFields());
    return clone;
}
#endif

void VMMethod::SetSignature(VMSymbol* sig) {
    VMInvokable::SetSignature(sig);
    SetNumberOfArguments(Signature::GetNumberOfArguments(sig));
}

#ifdef UNSAFE_FRAME_OPTIMIZATION
VMFrame* VMMethod::GetCachedFrame() const {
    return cachedFrame;
}

void VMMethod::SetCachedFrame(VMFrame* frame) {
    cachedFrame = frame;
    if (frame != nullptr) {
        frame->SetContext(nullptr);
        frame->SetBytecodeIndex(0);
        frame->ResetStackPointer();
#if GC_TYPE == GENERATIONAL
        _HEAP->WriteBarrier(this, cachedFrame);
#endif
    }
}
#endif

void VMMethod::SetNumberOfLocals(long nol) {
    store_ptr(numberOfLocals, NEW_INT(nol));
}

long VMMethod::GetMaximumNumberOfStackElements() {
    return INT_VAL(load_ptr(maximumNumberOfStackElements));
}

void VMMethod::SetMaximumNumberOfStackElements(long stel) {
    store_ptr(maximumNumberOfStackElements, NEW_INT(stel));
}

void VMMethod::SetNumberOfArguments(long noa) {
    store_ptr(numberOfArguments, NEW_INT(noa));
}

long VMMethod::GetNumberOfBytecodes() {
    return INT_VAL(load_ptr(bcLength));
}

void VMMethod::Invoke(Interpreter* interp, VMFrame* frame) {
    VMFrame* frm = interp->PushNewFrame(this);
    frm->CopyArgumentsFrom(frame);
}

void VMMethod::SetHolder(VMClass *hld) {
    VMInvokable::SetHolder(hld);
    SetHolderAll(hld);
}

void VMMethod::SetHolderAll(VMClass* hld) {
    long numIndexableFields = GetNumberOfIndexableFields();
    for (long i = 0; i < numIndexableFields; ++i) {
        vm_oop_t o = GetIndexableField(i);
        if (!IS_TAGGED(o)) {
            VMInvokable* vmi = dynamic_cast<VMInvokable*>(AS_OBJ(o));
            if (vmi != nullptr) {
                vmi->SetHolder(hld);
            }
        }
    }
}

vm_oop_t VMMethod::GetConstant(long indx) {
    uint8_t bc = bytecodes[indx + 1];
    if (bc >= GetNumberOfIndexableFields()) {
        Universe::ErrorPrint("Error: Constant index out of range\n");
        return nullptr;
    }
    return GetIndexableField(bc);
}

#if GC_TYPE==PAUSELESS
void VMMethod::MarkReferences() {
    VMInvokable::MarkReferences();
    
    ReadBarrierForGCThread(&numberOfLocals);
    ReadBarrierForGCThread(&maximumNumberOfStackElements);
    ReadBarrierForGCThread(&bcLength);
    ReadBarrierForGCThread(&numberOfArguments);
    long numIndexableFields = INT_VAL(ReadBarrierForGCThread(&numberOfConstants));
    
    for (long i = 0; i < numIndexableFields; ++i) {
        ReadBarrierForGCThread(&indexableFields[i]);
    }
}
void VMMethod::CheckMarking(void (*walk)(vm_oop_t)) {
    VMInvokable::CheckMarking(walk);
    assert(GetNMTValue(numberOfLocals) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(numberOfLocals));
    walk(Untag(numberOfLocals));
    assert(GetNMTValue(maximumNumberOfStackElements) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(maximumNumberOfStackElements));
    walk(Untag(maximumNumberOfStackElements));
    assert(GetNMTValue(bcLength) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(bcLength));
    walk(Untag(bcLength));
    assert(GetNMTValue(numberOfArguments) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(numberOfArguments));
    walk(Untag(numberOfArguments));
    assert(GetNMTValue(numberOfConstants) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(numberOfConstants));
    walk(Untag(numberOfConstants));
    long numIndexableFields = INT_VAL(Untag(numberOfConstants));
    for (long i = 0; i < numIndexableFields; ++i) {
        assert(GetNMTValue(AS_GC_POINTER(indexableFields[i])) == _HEAP->GetGCThread()->GetExpectedNMT());
        CheckBlocked(Untag(AS_GC_POINTER(indexableFields[i])));
        walk(Untag(AS_GC_POINTER(indexableFields[i])));
    }
}
#else
void VMMethod::WalkObjects(walk_heap_fn walk) {
    VMInvokable::WalkObjects(walk);
    
    numberOfLocals = (GCInteger*)(walk(numberOfLocals));
    maximumNumberOfStackElements = (GCInteger*)(walk(maximumNumberOfStackElements));
    bcLength = (GCInteger*)(walk(bcLength));
    numberOfArguments = (GCInteger*)(walk(numberOfArguments));
    numberOfConstants = (GCInteger*)(walk(numberOfConstants));
    
    /*
     #ifdef UNSAFE_FRAME_OPTIMIZATION
     if (cachedFrame != nullptr)
     cachedFrame = static_cast<VMFrame*>(walk(cachedFrame));
     #endif
     */
    
    long numIndexableFields = GetNumberOfIndexableFields();
    for (long i = 0; i < numIndexableFields; ++i) {
        if (GetIndexableField(i) != nullptr)
            indexableFields[i] = (GCAbstractObject*) walk(indexableFields[i]);
    }
}
#endif

void VMMethod::MarkObjectAsInvalid() {
    VMInvokable::MarkObjectAsInvalid();
    long numIndexableFields = INT_VAL(load_ptr(numberOfConstants));
    for (long i = 0; i < numIndexableFields; ++i) {
        indexableFields[i] = (GCAbstractObject*) INVALID_GC_POINTER;
    }
    numberOfConstants = (GCInteger*) INVALID_GC_POINTER;
    numberOfLocals = (GCInteger*) INVALID_GC_POINTER;
    maximumNumberOfStackElements = (GCInteger*) INVALID_GC_POINTER;
    bcLength = (GCInteger*) INVALID_GC_POINTER;
    numberOfArguments = (GCInteger*) INVALID_GC_POINTER;
}

StdString VMMethod::AsDebugString() {
    VMClass* holder = GetHolder();
    StdString holder_str;
    if (holder == load_ptr(nilObject)) {
        holder_str = "nil";
    } else {
        holder_str = holder->GetName()->GetStdString();
    }
    return "Method(" + holder_str + ">>#" + GetSignature()->GetStdString() + ")";
}
