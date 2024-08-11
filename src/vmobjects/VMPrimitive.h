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

#include "../primitivesCore/Primitives.h"
#include "PrimitiveRoutine.h"
#include "Signature.h"
#include "VMInvokable.h"
#include "VMObject.h"

class VMPrimitive : public VMInvokable {
public:
    typedef GCPrimitive Stored;

    static VMPrimitive* GetEmptyPrimitive(VMSymbol* sig, bool classSide);
    static VMPrimitive* GetFramePrim(VMSymbol* sig, FramePrim prim);

    VMPrimitive(VMSymbol* sig, FramePrim prim) : VMInvokable(sig), prim(prim) {
        write_barrier(this, sig);
    }

    VMClass* GetClass() const final { return load_ptr(primitiveClass); }

    inline size_t GetObjectSize() const override { return sizeof(VMPrimitive); }

    bool IsEmpty() const;

    inline void SetRoutine(FramePrim p) { prim = p; }

    VMPrimitive* CloneForMovingGC() const override;

    VMFrame* Invoke(VMFrame* frm) override {
        prim.pointer(frm);
        return nullptr;
    };

    void InlineInto(MethodGenerationContext& mgenc,
                    bool mergeScope = true) final;

    bool IsPrimitive() const override { return true; };

    void MarkObjectAsInvalid() override {
        VMInvokable::MarkObjectAsInvalid();
        prim.MarkObjectAsInvalid();
    }

    bool IsMarkedInvalid() const override { return !prim.IsValid(); }

    StdString AsDebugString() const override;

    inline size_t GetNumberOfArguments() const final {
        return Signature::GetNumberOfArguments(load_ptr(signature));
    }

private:
    make_testable(public);

    FramePrim prim;
};
