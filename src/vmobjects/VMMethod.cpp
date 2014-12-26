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
#include "../interpreter/Interpreter.h"

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
    cachedFrame = NULL;
#endif
#ifdef USE_TAGGING
    bcLength = TAG_INTEGER(bcCount);
    numberOfLocals = TAG_INTEGER(0);
    maximumNumberOfStackElements = TAG_INTEGER(0);
    numberOfArguments = TAG_INTEGER(0);
    this->numberOfConstants = TAG_INTEGER(numberOfConstants);
#else
    bcLength = WRITEBARRIER(_UNIVERSE->NewInteger(bcCount));
    numberOfLocals = WRITEBARRIER(_UNIVERSE->NewInteger(0));
    maximumNumberOfStackElements = WRITEBARRIER(_UNIVERSE->NewInteger(0));
    numberOfArguments = WRITEBARRIER(_UNIVERSE->NewInteger(0));
    this->numberOfConstants = WRITEBARRIER(_UNIVERSE->NewInteger(numberOfConstants));
#endif
    indexableFields = (GCAbstractObject**)(&indexableFields + 2);  // this is just a hack to get the convenience pointer, the fields start after the two other remaining fields in VMMethod
    for (long i = 0; i < numberOfConstants; ++i) {
        indexableFields[i] = WRITEBARRIER(READBARRIER(nilObject));
    }
    bytecodes = (uint8_t*)(&indexableFields + 2 + GetNumberOfIndexableFields());
}

#if GC_TYPE==GENERATIONAL
pVMMethod VMMethod::Clone() {
    pVMMethod clone = new (_HEAP, _PAGE, GetObjectSize() - sizeof(VMMethod), true)
    VMMethod(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this,
                                                             sizeof(VMObject)), GetObjectSize() -
           sizeof(VMObject));
    clone->indexableFields = (pVMObject*)(&(clone->indexableFields) + 2);
    clone->bytecodes = (uint8_t*)(&(clone->indexableFields) + 2 + GetNumberOfIndexableFields());
    return clone;
}
#elif GC_TYPE==PAUSELESS
pVMMethod VMMethod::Clone(Interpreter* thread) {
    pVMMethod clone = new (_HEAP, thread, GetObjectSize() - sizeof(VMMethod)) VMMethod(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this,
                                                             sizeof(VMObject)), GetObjectSize() -
           sizeof(VMObject));
    clone->indexableFields = (GCAbstractObject**)(&(clone->indexableFields) + 2);  // this is just a hack to get the convenience pointer, the fields start after the two other remaining fields in VMMethod
    clone->bytecodes = (uint8_t*)(&(clone->indexableFields) + 2 + GetNumberOfIndexableFields());
    /*clone->IncreaseVersion(); */
    return clone;
}
pVMMethod VMMethod::Clone(PauselessCollectorThread* thread) {
    pVMMethod clone = new (_HEAP, thread, GetObjectSize() - sizeof(VMMethod)) VMMethod(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this,
                                                             sizeof(VMObject)), GetObjectSize() -
           sizeof(VMObject));
    clone->indexableFields = (GCAbstractObject**)(&(clone->indexableFields) + 2);  // this is just a hack to get the convenience pointer, the fields start after the two other remaining fields in VMMethod
    clone->bytecodes = (uint8_t*)(&(clone->indexableFields) + 2 + ReadBarrierForGCThread(&numberOfConstants)->GetEmbeddedInteger());
    /*clone->IncreaseVersion(); */
    return clone;
}
#else
pVMMethod VMMethod::Clone() {
    pVMMethod clone = new (_HEAP, GetObjectSize() - sizeof(VMMethod))
    VMMethod(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this,
                                                             sizeof(VMObject)), GetObjectSize() -
           sizeof(VMObject));
    clone->indexableFields = (pVMObject*)(&(clone->indexableFields) + 2);
    clone->bytecodes = (uint8_t*)(&(clone->indexableFields) + 2 + GetNumberOfIndexableFields());
    return clone;
}
#endif

void VMMethod::SetSignature(pVMSymbol sig) {
    VMInvokable::SetSignature(sig);
    SetNumberOfArguments(Signature::GetNumberOfArguments(this->GetSignature()));
}

#ifdef UNSAFE_FRAME_OPTIMIZATION
pVMFrame VMMethod::GetCachedFrame() const {
    return cachedFrame;
}

void VMMethod::SetCachedFrame(pVMFrame frame) {
    cachedFrame = frame;
    if (frame != NULL) {
        frame->SetContext(NULL);
        frame->SetBytecodeIndex(0);
        frame->ResetStackPointer();
#if GC_TYPE == GENERATIONAL
        _HEAP->WriteBarrier(this, cachedFrame);
#endif
    }
}
#endif

void VMMethod::SetNumberOfLocals(long nol) {
#ifdef USE_TAGGING
    numberOfLocals = TAG_INTEGER(nol);
#else
    numberOfLocals = WRITEBARRIER(_UNIVERSE->NewInteger(nol));
#endif
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, numberOfLocals);
#endif
}

long VMMethod::GetMaximumNumberOfStackElements() {
#ifdef USE_TAGGING
    return UNTAG_INTEGER(maximumNumberOfStackElements);
#else
    return READBARRIER(maximumNumberOfStackElements)->GetEmbeddedInteger();
#endif
}

void VMMethod::SetMaximumNumberOfStackElements(long stel) {
#ifdef USE_TAGGING
    maximumNumberOfStackElements = TAG_INTEGER(stel);
#else
    maximumNumberOfStackElements = WRITEBARRIER(_UNIVERSE->NewInteger(stel));
#endif
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, maximumNumberOfStackElements);
#endif
}

void VMMethod::SetNumberOfArguments(long noa) {
#ifdef USE_TAGGING
    numberOfArguments = TAG_INTEGER(noa);
#else
    numberOfArguments = WRITEBARRIER(_UNIVERSE->NewInteger(noa));
#endif
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, numberOfArguments);
#endif
}

long VMMethod::GetNumberOfBytecodes() {
#ifdef USE_TAGGING
    return UNTAG_INTEGER(bcLength);
#else
    return READBARRIER(bcLength)->GetEmbeddedInteger();
#endif
}

void VMMethod::operator()(pVMFrame frame) {    
    pVMFrame frm = _UNIVERSE->GetInterpreter()->PushNewFrame(this);
    frm->CopyArgumentsFrom(frame);
}

void VMMethod::SetHolderAll(pVMClass hld) {
    long numIndexableFields = GetNumberOfIndexableFields();
    for (long i = 0; i < numIndexableFields; ++i) {
        pVMObject o = GetIndexableField(i);
        if (!IS_TAGGED(o)) {
            pVMInvokable vmi = dynamic_cast<pVMInvokable>(AS_VM_POINTER(o));
            if (vmi != NULL) {
                vmi->SetHolder(hld);
            }
        }
    }
}

pVMObject VMMethod::GetConstant(long indx) {
    uint8_t bc = bytecodes[indx + 1];
    if (bc >= this->GetNumberOfIndexableFields()) {
        cout << "Error: Constant index out of range" << endl;
        return NULL;
    }
    return this->GetIndexableField(bc);
}

#if GC_TYPE==PAUSELESS
void VMMethod::MarkReferences() {
    VMInvokable::MarkReferences();
    
    ReadBarrierForGCThread(&numberOfLocals);
    ReadBarrierForGCThread(&maximumNumberOfStackElements);
    ReadBarrierForGCThread(&bcLength);
    ReadBarrierForGCThread(&numberOfArguments);
    long numIndexableFields = ReadBarrierForGCThread(&numberOfConstants)->GetEmbeddedInteger();
    
    for (long i = 0; i < numIndexableFields; ++i) {
        ReadBarrierForGCThread(&indexableFields[i]);
    }
}
void VMMethod::CheckMarking(void (*walk)(AbstractVMObject*)) {
    VMInvokable::CheckMarking(walk);
    assert(GetNMTValue(numberOfLocals) == _HEAP->GetGCThread()->GetExpectedNMT());
    walk(Untag(numberOfLocals));
    assert(GetNMTValue(maximumNumberOfStackElements) == _HEAP->GetGCThread()->GetExpectedNMT());
    walk(Untag(maximumNumberOfStackElements));
    assert(GetNMTValue(bcLength) == _HEAP->GetGCThread()->GetExpectedNMT());
    walk(Untag(bcLength));
    assert(GetNMTValue(numberOfArguments) == _HEAP->GetGCThread()->GetExpectedNMT());
    walk(Untag(numberOfArguments));
    assert(GetNMTValue(numberOfConstants) == _HEAP->GetGCThread()->GetExpectedNMT());
    walk(Untag(numberOfConstants));
    long numIndexableFields = Untag(numberOfConstants)->GetEmbeddedInteger();
    for (long i = 0; i < numIndexableFields; ++i) {
        assert(GetNMTValue(AS_GC_POINTER(indexableFields[i])) == _HEAP->GetGCThread()->GetExpectedNMT());
        walk(Untag(AS_GC_POINTER(indexableFields[i])));
    }
}
#else
void VMMethod::WalkObjects(VMOBJECT_PTR (*walk)(VMOBJECT_PTR)) {
    VMInvokable::WalkObjects(walk);
    
    numberOfLocals = static_cast<VMInteger*>(walk(numberOfLocals));
    maximumNumberOfStackElements = static_cast<VMInteger*>(walk(maximumNumberOfStackElements));
    bcLength = static_cast<VMInteger*>(walk(bcLength));
    numberOfArguments = static_cast<VMInteger*>(walk(numberOfArguments));
    numberOfConstants = static_cast<VMInteger*>(walk(numberOfConstants));
    
    /*
     #ifdef UNSAFE_FRAME_OPTIMIZATION
     if (cachedFrame != NULL)
     cachedFrame = static_cast<VMFrame*>(walk(cachedFrame));
     #endif
     */
    
    long numIndexableFields = GetNumberOfIndexableFields();
    for (long i = 0; i < numIndexableFields; ++i) {
        if (GetIndexableField(i) != NULL)
            indexableFields[i] = walk(AS_POINTER(GetIndexableField(i)));
    }
}
#endif

void VMMethod::MarkObjectAsInvalid() {
    VMInvokable::MarkObjectAsInvalid();
    long numIndexableFields = Untag(numberOfConstants)->GetEmbeddedInteger();
    for (long i = 0; i < numIndexableFields; ++i) {
        indexableFields[i] = (GCAbstractObject*) INVALID_GC_POINTER;
    }
    numberOfConstants = (GCInteger*) INVALID_GC_POINTER;
    numberOfLocals = (GCInteger*) INVALID_GC_POINTER;
    maximumNumberOfStackElements = (GCInteger*) INVALID_GC_POINTER;
    bcLength = (GCInteger*) INVALID_GC_POINTER;
    numberOfArguments = (GCInteger*) INVALID_GC_POINTER;
}
