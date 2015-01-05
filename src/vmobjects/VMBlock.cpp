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
#include "VMEvaluationPrimitive.h"

#include "../vm/Universe.h"
#include "../interpreter/Interpreter.h"

const int VMBlock::VMBlockNumberOfFields = 2;

VMBlock::VMBlock() :
        VMObject(VMBlockNumberOfFields), blockMethod(), context() {
}

void VMBlock::SetMethod(pVMMethod bMethod) {
    blockMethod = WRITEBARRIER(bMethod);
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, bMethod);
#endif
}

#if GC_TYPE==GENERATIONAL
pVMBlock VMBlock::Clone() {
    return new (_HEAP, _PAGE, GetAdditionalSpaceConsumption(), true) VMBlock(*this);
}
#elif GC_TYPE==PAUSELESS
pVMBlock VMBlock::Clone(Interpreter* thread) {
    pVMBlock clone = new (_HEAP, thread, GetAdditionalSpaceConsumption()) VMBlock(*this);
    clone->IncreaseVersion();
    /* this->MarkObjectAsInvalid(); */
    return clone;
}
pVMBlock VMBlock::Clone(PauselessCollectorThread* thread) {
    pVMBlock clone = new (_HEAP, thread, GetAdditionalSpaceConsumption()) VMBlock(*this);
    clone->IncreaseVersion();
    /* this->MarkObjectAsInvalid(); */
    return clone;
}
#else
pVMBlock VMBlock::Clone() {
    return new (_HEAP, GetAdditionalSpaceConsumption()) VMBlock(*this);
}
#endif

pVMMethod VMBlock::GetMethod() {
    return READBARRIER(blockMethod);
}

void VMBlock::MarkObjectAsInvalid() {
    VMObject::MarkObjectAsInvalid();
    blockMethod = (GCMethod*)  INVALID_GC_POINTER;
    context = (GCFrame*) INVALID_GC_POINTER;
}
