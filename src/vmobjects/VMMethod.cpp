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

#include <compiler/MethodGenerationContext.h>
#include <vmobjects/IntegerBox.h>


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

    bcLength                     = NEW_INT(bcCount);
    numberOfLocals               = NEW_INT(0);
    maximumNumberOfStackElements = NEW_INT(0);
    numberOfArguments            = NEW_INT(0);
    this->numberOfConstants      = NEW_INT(numberOfConstants);

    indexableFields = (oop_t*)(&indexableFields + 2);
    for (long i = 0; i < numberOfConstants; ++i) {
        indexableFields[i] = nilObject;
    }
    bytecodes = (uint8_t*)(&indexableFields + 2 + GetNumberOfIndexableFields());
}

pVMMethod VMMethod::Clone() const {
    pVMMethod clone = new (GetHeap<HEAP_CLS>(), GetObjectSize() - sizeof(VMMethod) ALLOC_MATURE) VMMethod(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this,
                    sizeof(VMObject)), GetObjectSize() -
            sizeof(VMObject));
    clone->indexableFields = (oop_t*)(&(clone->indexableFields) + 2);
    clone->bytecodes = (uint8_t*)(&(clone->indexableFields) + 2 + GetNumberOfIndexableFields());
    return clone;
}

void VMMethod::SetSignature(pVMSymbol sig) {
    VMInvokable::SetSignature(sig);
    SetNumberOfArguments(Signature::GetNumberOfArguments(signature));
}

void VMMethod::WalkObjects(oop_t (*walk)(oop_t)) {
    VMInvokable::WalkObjects(walk);

    numberOfLocals = static_cast<VMInteger*>(walk(numberOfLocals));
    maximumNumberOfStackElements = static_cast<VMInteger*>(walk(maximumNumberOfStackElements));
    bcLength = static_cast<VMInteger*>(walk(bcLength));
    numberOfArguments = static_cast<VMInteger*>(walk(numberOfArguments));
    numberOfConstants = static_cast<VMInteger*>(walk(numberOfConstants));
#ifdef UNSAFE_FRAME_OPTIMIZATION
    if (cachedFrame != nullptr)
        cachedFrame = static_cast<VMFrame*>(walk(cachedFrame));
#endif

    long numIndexableFields = GetNumberOfIndexableFields();
    for (long i = 0; i < numIndexableFields; ++i) {
        if (GetIndexableField(i) != nullptr)
            indexableFields[i] = walk(GetIndexableField(i));
    }
}

#ifdef UNSAFE_FRAME_OPTIMIZATION
pVMFrame VMMethod::GetCachedFrame() const {
    return cachedFrame;
}

void VMMethod::SetCachedFrame(pVMFrame frame) {
    cachedFrame = frame;
    if (frame != nullptr) {
        frame->SetContext(nullptr);
        frame->SetBytecodeIndex(0);
        frame->ResetStackPointer();
        write_barrier(this, cachedFrame);
    }
}
#endif

void VMMethod::SetNumberOfLocals(long nol) {
    numberOfLocals = NEW_INT(nol);
    write_barrier(this, numberOfLocals);
}

long VMMethod::GetMaximumNumberOfStackElements() const {
    return INT_VAL(maximumNumberOfStackElements);
}

void VMMethod::SetMaximumNumberOfStackElements(long stel) {
    maximumNumberOfStackElements = NEW_INT(stel);
    write_barrier(this, maximumNumberOfStackElements);
}

void VMMethod::SetNumberOfArguments(long noa) {
    numberOfArguments = NEW_INT(noa);
    write_barrier(this, numberOfArguments);
}

long VMMethod::GetNumberOfBytecodes() const {
    return INT_VAL(bcLength);
}

void VMMethod::operator()(pVMFrame frame) {
    pVMFrame frm = GetUniverse()->GetInterpreter()->PushNewFrame(this);
    frm->CopyArgumentsFrom(frame);
}

void VMMethod::SetHolderAll(VMClass* hld) {
    long numIndexableFields = GetNumberOfIndexableFields();
    for (long i = 0; i < numIndexableFields; ++i) {
        oop_t o = GetIndexableField(i);
        if (!IS_TAGGED(o)) {
            pVMInvokable vmi = dynamic_cast<pVMInvokable>(AS_OBJ(o));
            if (vmi != nullptr) {
                vmi->SetHolder(hld);
            }
        }
    }
}

oop_t VMMethod::GetConstant(long indx) const {
    uint8_t bc = bytecodes[indx + 1];
    if (bc >= GetNumberOfIndexableFields()) {
        cout << "Error: Constant index out of range" << endl;
        return nullptr;
    }
    return GetIndexableField(bc);
}
