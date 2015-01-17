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

#include "VMEvaluationPrimitive.h"
#include "VMSymbol.h"
#include "VMObject.h"
#include "VMFrame.h"
#include "VMBlock.h"
#include "VMInteger.h"

#include <interpreter/Interpreter.h>
#include <vm/Universe.h>

//needed to instanciate the Routine object for the evaluation routine
#include <primitivesCore/Routine.h>

#include <vmobjects/VMBlock.inline.h>

VMEvaluationPrimitive::VMEvaluationPrimitive(long argc) : VMPrimitive(computeSignatureString(argc)) {
    SetRoutine(new EvaluationRoutine(this));
    SetEmpty(false);

    numberOfArguments = store_ptr(NEW_INT(argc));
}

#if GC_TYPE==GENERATIONAL
VMEvaluationPrimitive* VMEvaluationPrimitive::Clone() {
    return new (_HEAP, _PAGE, 0, true) VMEvaluationPrimitive(*this);
}
#elif GC_TYPE==PAUSELESS
VMEvaluationPrimitive* VMEvaluationPrimitive::Clone(Interpreter* thread) {
    VMEvaluationPrimitive* clone = new (_HEAP, thread) VMEvaluationPrimitive(*this);
    /* clone->IncreaseVersion();
    this->MarkObjectAsInvalid(); */
    return clone;
}
VMEvaluationPrimitive* VMEvaluationPrimitive::Clone(PauselessCollectorThread* thread) {
    VMEvaluationPrimitive* clone = new (_HEAP, thread) VMEvaluationPrimitive(*this);
    /* clone->IncreaseVersion();
    this->MarkObjectAsInvalid(); */
    return clone;
}
#else
VMEvaluationPrimitive* VMEvaluationPrimitive::Clone() {
    return new (_HEAP) VMEvaluationPrimitive(*this);
}
#endif
    
VMSymbol* VMEvaluationPrimitive::computeSignatureString(long argc) {
    assert(argc > 0);

    StdString signatureString;

    // Compute the signature string
    if (argc==1) {
        signatureString += "value";
    } else {
        signatureString += "value:";
        --argc;
        while (--argc)
            // Add extra value: selector elements if necessary
            signatureString += "with:";
    }

    // Return the signature string
    return GetUniverse()->SymbolFor(signatureString);
}

void EvaluationRoutine::Invoke(Interpreter* interp, VMFrame* frame) {
    VMEvaluationPrimitive* prim = load_ptr(evalPrim);

    // Get the block (the receiver) from the stack
    long numArgs = prim->GetNumberOfArguments();
    VMBlock* block = static_cast<VMBlock*>(frame->GetStackElement(numArgs - 1));

    // Get the context of the block...
    VMFrame* context = block->GetContext();

    // Push a new frame and set its context to be the one specified in the block
    VMFrame* NewFrame = interp->PushNewFrame(block->GetMethod());
    NewFrame->CopyArgumentsFrom(frame);
    NewFrame->SetContext(context);
}

void VMEvaluationPrimitive::MarkObjectAsInvalid() {
    VMPrimitive::MarkObjectAsInvalid();
    numberOfArguments = (GCInteger*)  INVALID_GC_POINTER;
}

#if GC_TYPE==PAUSELESS
void VMEvaluationPrimitive::MarkReferences() {
    VMPrimitive::MarkReferences();
    ReadBarrierForGCThread(&numberOfArguments);
}
void VMEvaluationPrimitive::CheckMarking(void (*walk)(vm_oop_t)) {
    VMPrimitive::CheckMarking(walk);
    assert(GetNMTValue(numberOfArguments) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(numberOfArguments));
    walk(Untag(numberOfArguments));
}
#else
void VMEvaluationPrimitive::WalkObjects(VMOBJECT_PTR (*walk)(VMOBJECT_PTR)) {
    VMPrimitive::WalkObjects(walk);
    numberOfArguments = (GCInteger*) (walk(load_ptr(numberOfArguments)));
}
#endif

#if GC_TYPE==PAUSELESS
void EvaluationRoutine::MarkReferences() {
    ReadBarrierForGCThread(&evalPrim);
}
void EvaluationRoutine::CheckMarking(void (*walk)(vm_oop_t)) {
    assert(GetNMTValue(evalPrim) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(evalPrim));
    walk(Untag(evalPrim));
}
#endif

StdString VMEvaluationPrimitive::AsDebugString() {
    return "VMEvaluationPrimitive(" + to_string(
                    INT_VAL(load_ptr(numberOfArguments))) + ")";
}
