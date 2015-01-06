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

#include "../vm/Universe.h"

//needed to instanciate the Routine object for the evaluation routine
#include "../primitivesCore/Routine.h"

VMEvaluationPrimitive::VMEvaluationPrimitive(long argc) : VMPrimitive(computeSignatureString(argc)) {
    SetRoutine(new EvaluationRoutine(this));
    SetEmpty(false);
    store_ptr(numberOfArguments, NEW_INT(argc));
}

VMEvaluationPrimitive* VMEvaluationPrimitive::Clone() const {
    VMEvaluationPrimitive* evPrim = new (GetHeap<HEAP_CLS>(), 0 ALLOC_MATURE) VMEvaluationPrimitive(*this);
    return evPrim;
}

void VMEvaluationPrimitive::WalkObjects(walk_heap_fn walk) {
    VMPrimitive::WalkObjects(walk);
    numberOfArguments = walk(numberOfArguments);
    static_cast<EvaluationRoutine*>(routine)->WalkObjects(walk);
}

VMSymbol* VMEvaluationPrimitive::computeSignatureString(long argc) {
#define VALUE_S "value"
#define VALUE_LEN 5
#define WITH_S    "with:"
#define WITH_LEN (4+1)
#define COLON_S ":"
    assert(argc > 0);

    StdString signatureString;

    // Compute the signature string
    if (argc==1) {
        signatureString += VALUE_S;
    } else {
        signatureString += VALUE_S;
        signatureString += COLON_S;
        --argc;
        while (--argc)
            // Add extra value: selector elements if necessary
            signatureString += WITH_S;
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

void EvaluationRoutine::WalkObjects(walk_heap_fn walk) {
    evalPrim = static_cast<GCEvaluationPrimitive*>(walk(evalPrim));
}

StdString VMEvaluationPrimitive::AsDebugString() const {
    return "VMEvaluationPrimitive(" + to_string(
                    INT_VAL(load_ptr(numberOfArguments))) + ")";
}
