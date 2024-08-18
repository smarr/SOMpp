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

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>

#include "../compiler/LexicalScope.h"
#include "../memory/Heap.h"
#include "../misc/defs.h"
#include "../vm/Print.h"
#include "../vm/Symbols.h"
#include "../vm/Universe.h"  // NOLINT(misc-include-cleaner) it's required to make the types complete
#include "ObjectFormats.h"
#include "VMBlock.h"
#include "VMFrame.h"
#include "VMSymbol.h"

VMEvaluationPrimitive::VMEvaluationPrimitive(uint8_t argc)
    : VMInvokable(computeSignatureString(argc)), numberOfArguments(argc) {
    write_barrier(this, load_ptr(signature));
}

VMEvaluationPrimitive* VMEvaluationPrimitive::CloneForMovingGC() const {
    auto* evPrim =
        new (GetHeap<HEAP_CLS>(), 0 ALLOC_MATURE) VMEvaluationPrimitive(*this);
    return evPrim;
}

void VMEvaluationPrimitive::WalkObjects(walk_heap_fn walk) {
    VMInvokable::WalkObjects(walk);
}

VMSymbol* VMEvaluationPrimitive::computeSignatureString(size_t argc) {
#define VALUE_S "value"
#define VALUE_LEN 5
#define WITH_S "with:"
#define WITH_LEN (4 + 1)
#define COLON_S ":"
    assert(argc > 0);

    std::string signatureString;

    // Compute the signature string
    if (argc == 1) {
        signatureString += VALUE_S;
    } else {
        signatureString += VALUE_S;
        signatureString += COLON_S;
        --argc;
        while (--argc != 0) {
            // Add extra value: selector elements if necessary
            signatureString += WITH_S;
        }
    }

    // Return the signature string
    return SymbolFor(signatureString);
}

VMFrame* VMEvaluationPrimitive::Invoke(VMFrame* frm) {
    // Get the block (the receiver) from the stack
    auto* block =
        static_cast<VMBlock*>(frm->GetStackElement(numberOfArguments - 1));

    // Get the context of the block...
    VMFrame* context = block->GetContext();
    VMInvokable* method = block->GetMethod();
    VMFrame* newFrame = method->Invoke(frm);

    // Push set its context to be the one specified in the block
    if (newFrame != nullptr) {
        newFrame->SetContext(context);
    }
    return nullptr;
}

VMFrame* VMEvaluationPrimitive::Invoke1(VMFrame* frm) {
    assert(numberOfArguments == 1);
    // Get the block (the receiver) from the stack
    auto* block = static_cast<VMBlock*>(frm->GetStackElement(0));

    // Get the context of the block...
    VMFrame* context = block->GetContext();
    VMInvokable* method = block->GetMethod();
    VMFrame* newFrame = method->Invoke1(frm);

    // Push set its context to be the one specified in the block
    if (newFrame != nullptr) {
        newFrame->SetContext(context);
    }
    return nullptr;
}

std::string VMEvaluationPrimitive::AsDebugString() const {
    return "VMEvaluationPrimitive(" + to_string(numberOfArguments) + ")";
}

#define INVALID_INT_MARKER 9002002002002002002

void VMEvaluationPrimitive::MarkObjectAsInvalid() {
    VMInvokable::MarkObjectAsInvalid();
    numberOfArguments = INVALID_INT_MARKER;
}

bool VMEvaluationPrimitive::IsMarkedInvalid() const {
    return numberOfArguments == INVALID_INT_MARKER;
}

void VMEvaluationPrimitive::InlineInto(MethodGenerationContext& /*mgenc*/,
                                       bool /*mergeScope*/) {
    ErrorExit(
        "VMEvaluationPrimitive::InlineInto is not supported, and should not be "
        "reached");
}
