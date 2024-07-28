#pragma once

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

#include "VMPrimitive.h"

class VMEvaluationPrimitive: public VMPrimitive {
public:
    typedef GCEvaluationPrimitive Stored;

    VMEvaluationPrimitive(size_t argc);
    void WalkObjects(walk_heap_fn) override;
    VMEvaluationPrimitive* CloneForMovingGC() const override;

    StdString AsDebugString() const override;

    int64_t GetNumberOfArguments() { return numberOfArguments; }
    
    inline size_t GetObjectSize() const override {
        return sizeof(VMEvaluationPrimitive);
    }

    void MarkObjectAsInvalid() override;
    bool IsMarkedInvalid() const override;

private:
    static VMSymbol* computeSignatureString(long argc);
    void evaluationRoutine(Interpreter*, VMFrame*);
private_testable:
    size_t numberOfArguments;

};

class EvaluationRoutine : public PrimitiveRoutine {
private:
    GCEvaluationPrimitive* evalPrim;
public:
    EvaluationRoutine(VMEvaluationPrimitive* prim)
        : PrimitiveRoutine(),
            // the store without barrier is fine here,
            // because it's a cyclic structure with `prim` itself,
            // which will be store in another object,
            // which will then have a barrier
            evalPrim(store_with_separate_barrier(prim)) {};
    void WalkObjects(walk_heap_fn);
    bool isClassSide() override { return false; }
    void Invoke(Interpreter* interp, VMFrame* frame) override;
};
