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

const int VMBlock::VMBlockNumberOfFields = 2;

VMBlock::VMBlock() :
        VMObject(VMBlockNumberOfFields), blockMethod(), context() {
}

void VMBlock::SetMethod(pVMMethod bMethod) {
    blockMethod = (bMethod);
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, bMethod);
#endif
}

pVMBlock VMBlock::Clone() /*const*/ {
    pVMBlock clone;
#if GC_TYPE==GENERATIONAL
    clone = new (_HEAP, _PAGE, GetAdditionalSpaceConsumption(), true) VMBlock(*this);
#elif GC_TYPE==PAUSELESS
    clone = new (_PAGE, GetAdditionalSpaceConsumption()) VMBlock(*this);
#else
    clone = new (_HEAP, GetAdditionalSpaceConsumption()) VMBlock(*this);
#endif
    return clone;
}

pVMMethod VMBlock::GetMethod() /*const*/ {
    PG_HEAP(ReadBarrier((void**)(&blockMethod)));
    return (blockMethod);
}

pVMEvaluationPrimitive VMBlock::GetEvaluationPrimitive(int numberOfArguments) {
#if GC_TYPE==GENERATIONAL
    return new (_HEAP, _PAGE) VMEvaluationPrimitive(numberOfArguments);
#elif GC_TYPE==PAUSELESS
    return new (_PAGE) VMEvaluationPrimitive(numberOfArguments);
#else
    return new (_HEAP) VMEvaluationPrimitive(numberOfArguments);
#endif
}
