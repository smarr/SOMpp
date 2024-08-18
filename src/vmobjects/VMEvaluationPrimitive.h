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

class VMEvaluationPrimitive : public VMInvokable {
public:
    typedef GCEvaluationPrimitive Stored;

    VMEvaluationPrimitive(uint8_t argc);
    void WalkObjects(walk_heap_fn /*unused*/) override;
    VMEvaluationPrimitive* CloneForMovingGC() const override;

    std::string AsDebugString() const override;

    inline size_t GetObjectSize() const override {
        return sizeof(VMEvaluationPrimitive);
    }

    VMClass* GetClass() const final { return load_ptr(primitiveClass); }

    void MarkObjectAsInvalid() override;
    bool IsMarkedInvalid() const override;

    VMFrame* Invoke(VMFrame* frm) override;
    VMFrame* Invoke1(VMFrame* frm) override;
    void InlineInto(MethodGenerationContext& mgenc,
                    bool mergeScope = true) final;

    inline uint8_t GetNumberOfArguments() const final {
        return numberOfArguments;
    }

private:
    static VMSymbol* computeSignatureString(size_t argc);
    void evaluationRoutine(VMFrame*);

    make_testable(public);

    uint8_t numberOfArguments;
};
