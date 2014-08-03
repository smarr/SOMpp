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

#include "VMMethod.h"
#include "VMFrame.h"
#include "VMClass.h"
#include "VMSymbol.h"
#include "VMArray.h"
#include "VMObject.h"
#include "VMInteger.h"
#include "Signature.h"

#include <vm/Universe.h>

//#include <compiler/MethodGenerationContext.h>


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
    bcLength = _UNIVERSE->NewInteger(bcCount);
    numberOfLocals = _UNIVERSE->NewInteger(0);
    maximumNumberOfStackElements = _UNIVERSE->NewInteger(0);
    numberOfArguments = _UNIVERSE->NewInteger(0);
    this->numberOfConstants = _UNIVERSE->NewInteger(numberOfConstants);
#endif
    indexableFields = (pVMObject*)(&indexableFields + 2);
    for (long i = 0; i < numberOfConstants; ++i) {
        PG_HEAP(ReadBarrier((void**)(&nilObject)));
        indexableFields[i] = nilObject;
    }
    bytecodes = (uint8_t*)(&indexableFields + 2 + GetNumberOfIndexableFields());
}

pVMMethod VMMethod::Clone() /*const*/ {
#if GC_TYPE==GENERATIONAL
    pVMMethod clone = new (_HEAP, _PAGE, GetObjectSize() - sizeof(VMMethod), true)
#elif GC_TYPE==PAUSELESS
    pVMMethod clone = new (_PAGE, GetObjectSize() - sizeof(VMMethod))
#else
    pVMMethod clone = new (_HEAP, GetObjectSize() - sizeof(VMMethod))
#endif
    VMMethod(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this,
                    sizeof(VMObject)), GetObjectSize() -
            sizeof(VMObject));
    clone->indexableFields = (pVMObject*)(&(clone->indexableFields) + 2);
    clone->bytecodes = (uint8_t*)(&(clone->indexableFields) + 2 + GetNumberOfIndexableFields());
    return clone;
}

void VMMethod::SetSignature(pVMSymbol sig) {
    VMInvokable::SetSignature(sig);
    SetNumberOfArguments(Signature::GetNumberOfArguments(signature));
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
    numberOfLocals = _UNIVERSE->NewInteger(nol);
#endif
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, numberOfLocals);
#endif
}

long VMMethod::GetMaximumNumberOfStackElements() const {
#ifdef USE_TAGGING
    return UNTAG_INTEGER(maximumNumberOfStackElements);
#else
    PG_HEAP(ReadBarrier((void**)(&maximumNumberOfStackElements)));
    return maximumNumberOfStackElements->GetEmbeddedInteger();
#endif
}

void VMMethod::SetMaximumNumberOfStackElements(long stel) {
#ifdef USE_TAGGING
    maximumNumberOfStackElements = TAG_INTEGER(stel);
#else
    maximumNumberOfStackElements = _UNIVERSE->NewInteger(stel);
#endif
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, maximumNumberOfStackElements);
#endif
}

void VMMethod::SetNumberOfArguments(long noa) {
#ifdef USE_TAGGING
    numberOfArguments = TAG_INTEGER(noa);
#else
    numberOfArguments = _UNIVERSE->NewInteger(noa);
#endif
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, numberOfArguments);
#endif
}

long VMMethod::GetNumberOfBytecodes() const {
#ifdef USE_TAGGING
    return UNTAG_INTEGER(bcLength);
#else
    PG_HEAP(ReadBarrier((void**)(&bcLength)));
    return bcLength->GetEmbeddedInteger();
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
            pVMInvokable vmi = dynamic_cast<pVMInvokable>(AS_POINTER(o));
            if (vmi != NULL) {
                vmi->SetHolder(hld);
            }
        }
    }
}

pVMObject VMMethod::GetConstant(long indx) /*const*/ {
    uint8_t bc = bytecodes[indx + 1];
    if (bc >= this->GetNumberOfIndexableFields()) {
        cout << "Error: Constant index out of range" << endl;
        return NULL;
    }
    return this->GetIndexableField(bc);
}

#if GC_TYPE==PAUSELESS
void VMMethod::MarkReferences(Worklist* worklist) {
    VMInvokable::MarkReferences(worklist);
    
    worklist->PushFront(numberOfLocals);
    worklist->PushFront(maximumNumberOfStackElements);
    worklist->PushFront(bcLength);
    worklist->PushFront(numberOfArguments);
    worklist->PushFront(numberOfConstants);
    
    long numIndexableFields = GetNumberOfIndexableFields();
    for (long i = 0; i < numIndexableFields; ++i) {
        if (GetIndexableField(i) != NULL)
            worklist->PushFront(AS_POINTER(GetIndexableField(i)));
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
