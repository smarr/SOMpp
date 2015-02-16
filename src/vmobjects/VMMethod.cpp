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

#include <vm/Universe.h>
#include <vmobjects/VMBlock.inline.h>
#include <vmobjects/IntegerBox.h>
#include <vmobjects/VMMethod.inline.h>

#ifdef UNSAFE_FRAME_OPTIMIZATION
const size_t VMMethod::VMMethodNumberOfGcPtrFields = 6;
#else
const size_t VMMethod::VMMethodNumberOfGcPtrFields = 5;
#endif

VMMethod::VMMethod(size_t bcCount, size_t numberOfConstants, size_t nof, Page* page) :
        VMInvokable(nof + VMMethodNumberOfGcPtrFields) {
#ifdef UNSAFE_FRAME_OPTIMIZATION
    cachedFrame = nullptr;
#endif
# warning, if we use extra parts of the heap for the allocation, we probably need to trigger the generational barrier
    bcLength                     = to_gc_ptr(NEW_INT(bcCount, page));
    numberOfLocals               = to_gc_ptr(NEW_INT(0, page));
    maximumNumberOfStackElements = to_gc_ptr(NEW_INT(0, page));
    numberOfArguments            = to_gc_ptr(NEW_INT(0, page));
    this->numberOfConstants      = to_gc_ptr(NEW_INT(numberOfConstants, page));

    indexableFields = (gc_oop_t*)(&indexableFields + 2);  // this is just a hack to get the convenience pointer, the fields start after the two other remaining fields in VMMethod
    for (size_t i = 0; i < numberOfConstants; ++i) {
        indexableFields[i] = nilObject;
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

void VMMethod::WalkObjects(walk_heap_fn walk, Page* page) {
    VMInvokable::WalkObjects(walk, page);

    int64_t numIndexableFields = GetNumberOfIndexableFields();
    for (size_t i = 0; i < numIndexableFields; ++i) {
        do_walk(indexableFields[i]);
    }
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
#warning this check looks incredibly slow
        Universe::ErrorPrint("Error: Constant index out of range\n");
        return nullptr;
    }
    return GetIndexableField(bc);
}

void VMMethod::MarkObjectAsInvalid() {
    int64_t numIndexableFields = INT_VAL(load_ptr(numberOfConstants)); // load value before marking stuff invalid
    VMInvokable::MarkObjectAsInvalid();

    for (size_t i = 0; i < numIndexableFields; ++i) {
        indexableFields[i] = (GCAbstractObject*) INVALID_GC_POINTER;
    }
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
