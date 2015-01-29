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

VMMethod::VMMethod(long bcCount, long numberOfConstants, long nof, Page* page) :
        VMInvokable(nof + VMMethodNumberOfFields) {
#ifdef UNSAFE_FRAME_OPTIMIZATION
    cachedFrame = nullptr;
#endif
# warning not sure whether the use of _store_ptr is ok here
# warning, if we use extra parts of the heap for the allocation, we probably need to trigger the generational barrier
    bcLength                     = _store_ptr(NEW_INT(bcCount, page));
    numberOfLocals               = _store_ptr(NEW_INT(0, page));
    maximumNumberOfStackElements = _store_ptr(NEW_INT(0, page));
    numberOfArguments            = _store_ptr(NEW_INT(0, page));
    this->numberOfConstants      = _store_ptr(NEW_INT(numberOfConstants, page));

    indexableFields = (gc_oop_t*)(&indexableFields + 2);  // this is just a hack to get the convenience pointer, the fields start after the two other remaining fields in VMMethod
    for (long i = 0; i < numberOfConstants; ++i) {
#warning do we need to cylce through the barriers here?
        store_ptr(indexableFields[i], load_ptr(nilObject));
    }
    bytecodes = (uint8_t*)(&indexableFields + 2 + GetNumberOfIndexableFields());
}

VMMethod* VMMethod::Clone(Page* page) {
    VMMethod* clone = new (page, GetObjectSize() - sizeof(VMMethod) ALLOC_MATURE) VMMethod(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)),
           SHIFTED_PTR(this,  sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    clone->indexableFields = (gc_oop_t*)(&(clone->indexableFields) + 2);
    clone->bytecodes = (uint8_t*)(&(clone->indexableFields) + 2 + GetNumberOfIndexableFields());
    return clone;
}

void VMMethod::SetSignature(VMSymbol* sig, Page* page) {
    VMInvokable::SetSignature(sig);
    SetNumberOfArguments(Signature::GetNumberOfArguments(sig), page);
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
        assert(GetNMTValue(indexableFields[i]) == _HEAP->GetGCThread()->GetExpectedNMT());
        CheckBlocked(Untag(indexableFields[i]));
        walk(Untag(indexableFields[i]));
    }
}
#else
void VMMethod::WalkObjects(walk_heap_fn walk, Page* page) {
    VMInvokable::WalkObjects(walk, page);

    numberOfLocals    = walk(numberOfLocals, page);
    maximumNumberOfStackElements = walk(maximumNumberOfStackElements, page);
    bcLength          = walk(bcLength, page);
    numberOfArguments = walk(numberOfArguments, page);
    numberOfConstants = walk(numberOfConstants, page);

    /*
#ifdef UNSAFE_FRAME_OPTIMIZATION
    if (cachedFrame != nullptr)
        cachedFrame = static_cast<VMFrame*>(walk(cachedFrame, page));
#endif
     */

    long numIndexableFields = GetNumberOfIndexableFields();
    for (long i = 0; i < numIndexableFields; ++i) {
        if (GetIndexableField(i) != nullptr)
            indexableFields[i] = (GCAbstractObject*) walk(indexableFields[i], page);
    }
}
#endif

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
        write_barrier(this, cachedFrame);
    }
}
#endif

void VMMethod::SetNumberOfLocals(long nol, Page* page) {
    store_ptr(numberOfLocals, NEW_INT(nol, page));
}

long VMMethod::GetMaximumNumberOfStackElements() {
    return INT_VAL(load_ptr(maximumNumberOfStackElements));
}

void VMMethod::SetMaximumNumberOfStackElements(long stel, Page* page) {
    store_ptr(maximumNumberOfStackElements, NEW_INT(stel, page));
}

void VMMethod::SetNumberOfArguments(long noa, Page* page) {
    store_ptr(numberOfArguments, NEW_INT(noa, page));
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
