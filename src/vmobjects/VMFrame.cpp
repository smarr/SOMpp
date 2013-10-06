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

#include "VMFrame.h"
#include "VMMethod.h"
#include "VMObject.h"
#include "VMInteger.h"
#include "VMClass.h"
#include "VMSymbol.h"

#include "../vm/Universe.h"

//when doesNotUnderstand or UnknownGlobal is sent, additional stack slots might
//be necessary, as these cases are not taken into account when the stack
//depth is calculated. In that case this method is called.
pVMFrame VMFrame::EmergencyFrameFrom( pVMFrame from, long extraLength ) {
    long length = from->GetNumberOfIndexableFields() + extraLength;
    long additionalBytes = length * sizeof(pVMObject);
    pVMFrame result = new (_HEAP, additionalBytes) VMFrame(length);

    result->SetClass(from->GetClass());
    //copy arguments, locals and the stack
    from->CopyIndexableFieldsTo(result);

    //set Frame members
    result->SetPreviousFrame(from->GetPreviousFrame());
    result->SetMethod(from->GetMethod());
    result->SetContext(from->GetContext());
    result->stackPointer = from->GetStackPointer();
    result->bytecodeIndex = from->bytecodeIndex;
    result->localOffset = from->localOffset;

    return result;
}

const long VMFrame::VMFrameNumberOfFields = 6;

VMFrame::VMFrame(long size, long nof) :
        VMArray(size, nof + VMFrameNumberOfFields) {
    _HEAP->StartUninterruptableAllocation();
    this->localOffset = _UNIVERSE->NewInteger(0);
    this->bytecodeIndex = _UNIVERSE->NewInteger(0);
    this->stackPointer = _UNIVERSE->NewInteger(0);
    _HEAP->EndUninterruptableAllocation();
}

void VMFrame::SetMethod(pVMMethod method) {
    this->method = method;
}

pVMFrame VMFrame::GetContextLevel(long lvl) const {
    const pVMFrame current = this;
    while (lvl > 0) {
        current = current->GetContext();
        --lvl;
    }
    return const_cast<pVMFrame>(current);
}

pVMFrame VMFrame::GetOuterContext() const {
    const pVMFrame current = this;
    while (current->HasContext()) {
        current = current->GetContext();
    }
    return const_cast<pVMFrame>(current);
}

long VMFrame::RemainingStackSize() const {
    // - 1 because the stack pointer points at the top entry,
    // so the next entry would be put at stackPointer+1
    return this->GetNumberOfIndexableFields()
            - stackPointer->GetEmbeddedInteger() - 1;
}

pVMObject VMFrame::Pop() {
    int32_t sp = this->stackPointer->GetEmbeddedInteger();
    this->stackPointer->SetEmbeddedInteger(sp-1);
    return GetIndexableField(sp);
}

void VMFrame::Push(pVMObject obj) {
    int32_t sp = this->stackPointer->GetEmbeddedInteger() + 1;
    this->stackPointer->SetEmbeddedInteger(sp);
    SetIndexableField(sp, obj);
}

void VMFrame::PrintStack() const {
    cout << "SP: " << this->stackPointer->GetEmbeddedInteger() << endl;
    long numStackElements = GetNumberOfIndexableFields() + 1;
    for (int i = 0; i < numStackElements; ++i) {
        pVMObject vmo = GetIndexableField(i);
        cout << i << ": ";
        if (vmo == NULL)
        cout << "NULL" << endl;
        if (vmo == nilObject)
        cout << "NIL_OBJECT" << endl;
        if (vmo->GetClass() == NULL)
        cout << "VMObject with Class == NULL" << endl;
        if (vmo->GetClass() == nilObject)
        cout << "VMObject with Class == NIL_OBJECT" << endl;
        else
        cout << "index: " << i << " object:"
        << vmo->GetClass()->GetName()->GetChars() << endl;
    }
}

void VMFrame::ResetStackPointer() {
    // arguments are stored in front of local variables
    pVMMethod meth = this->GetMethod();
    size_t lo = meth->GetNumberOfArguments();
    this->localOffset->SetEmbeddedInteger(lo);

    // Set the stack pointer to its initial value thereby clearing the stack
    size_t numLocals = meth->GetNumberOfLocals();
    this->stackPointer->SetEmbeddedInteger(lo + numLocals - 1);
}

pVMObject VMFrame::GetStackElement(long index) const {
    int sp = this->stackPointer->GetEmbeddedInteger();
    return GetIndexableField(sp - index);
}

void VMFrame::SetStackElement(long index, pVMObject obj) {
    int sp = this->stackPointer->GetEmbeddedInteger();
    SetIndexableField(sp - index, obj);
}

pVMObject VMFrame::GetLocal(long index, long contextLevel) const {
    pVMFrame context = this->GetContextLevel(contextLevel);
    long lo = context->localOffset->GetEmbeddedInteger();
    return context->GetIndexableField(lo + index);
}

void VMFrame::SetLocal(long index, long contextLevel, pVMObject value) {
    pVMFrame context = this->GetContextLevel(contextLevel);
    size_t lo = context->localOffset->GetEmbeddedInteger();
    context->SetIndexableField(lo + index, value);
}

pVMObject VMFrame::GetArgument(long index, long contextLevel) const {
    // get the context
    pVMFrame context = this->GetContextLevel(contextLevel);
    return context->GetIndexableField(index);
}

void VMFrame::SetArgument(long index, long contextLevel, pVMObject value) {
    pVMFrame context = this->GetContextLevel(contextLevel);
    context->SetIndexableField(index, value);
}

void VMFrame::PrintStackTrace() const {
    //TODO
}

long VMFrame::ArgumentStackIndex(long index) const {
    pVMMethod meth = this->GetMethod();
    return meth->GetNumberOfArguments() - index - 1;
}

void VMFrame::CopyArgumentsFrom(pVMFrame frame) {
    // copy arguments from frame:
    // - arguments are at the top of the stack of frame.
    // - copy them into the argument area of the current frame
    pVMMethod meth = this->GetMethod();
    long num_args = meth->GetNumberOfArguments();
    for (long i = 0; i < num_args; ++i) {
        pVMObject stackElem = frame->GetStackElement(num_args - 1 - i);
        SetIndexableField(i, stackElem);
    }
}

void VMFrame::MarkReferences() {
    if (gcfield)
        return;
    VMArray::MarkReferences();
}
