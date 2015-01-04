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

void VMBlock::SetMethod(VMMethod* bMethod) {
    store_ptr(blockMethod, bMethod);
}

VMBlock* VMBlock::Clone() const {
    VMBlock* clone;
    clone = new (GetHeap<HEAP_CLS>(), GetAdditionalSpaceConsumption() ALLOC_MATURE) VMBlock(*this);
    return clone;
}

VMMethod* VMBlock::GetMethod() const {
    return load_ptr(blockMethod);
}

VMEvaluationPrimitive* VMBlock::GetEvaluationPrimitive(int numberOfArguments) {
    return new (GetHeap<HEAP_CLS>()) VMEvaluationPrimitive(numberOfArguments);
}

StdString VMBlock::AsDebugString() const {
    return "Block(" + load_ptr(blockMethod)->AsDebugString() + ")";
}
