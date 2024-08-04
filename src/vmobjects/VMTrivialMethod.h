#pragma once

#include "../compiler/MethodGenerationContext.h"
#include "../vm/Globals.h"
#include "Signature.h"
#include "VMInvokable.h"

class VMTrivialMethod : public VMInvokable {
public:
    typedef GCTrivialMethod Stored;

    VMTrivialMethod(VMSymbol* sig, vector<Variable>& arguments)
        : VMInvokable(sig), arguments(arguments) {}

    VMClass* GetClass() const final { return load_ptr(methodClass); }

    bool IsPrimitive() const final { return false; };

    void MergeScopeInto(MethodGenerationContext& mgenc) final {
        if (arguments.size() > 0) {
            mgenc.InlineAsLocals(arguments);
        }
    }

    const Variable* GetArgument(size_t index, size_t contextLevel) final {
        assert(contextLevel == 0);
        if (contextLevel > 0) {
            return nullptr;
        }
        return &arguments.at(index);
    }

    inline size_t GetNumberOfArguments() const final {
        return Signature::GetNumberOfArguments(load_ptr(signature));
    }

private:
    vector<Variable> arguments;
};

VMTrivialMethod* MakeLiteralReturn(VMSymbol* sig, vector<Variable>& arguments,
                                   vm_oop_t literal);

class VMLiteralReturn : public VMTrivialMethod {
public:
    typedef GCLiteralReturn Stored;

    VMLiteralReturn(VMSymbol* sig, vector<Variable>& arguments,
                    vm_oop_t literal)
        : VMTrivialMethod(sig, arguments),
          literal(store_with_separate_barrier(literal)),
          numberOfArguments(Signature::GetNumberOfArguments(sig)) {
        write_barrier(this, sig);
        write_barrier(this, literal);
    }

    inline size_t GetObjectSize() const override {
        return sizeof(VMLiteralReturn);
    }

    VMFrame* Invoke(Interpreter*, VMFrame*) override;
    void InlineInto(MethodGenerationContext& mgenc,
                    bool mergeScope = true) final;

    AbstractVMObject* CloneForMovingGC() const final;

    void MarkObjectAsInvalid() final {
        VMTrivialMethod::MarkObjectAsInvalid();
        literal = INVALID_GC_POINTER;
    }

    void WalkObjects(walk_heap_fn) override;

    bool IsMarkedInvalid() const final { return literal == INVALID_GC_POINTER; }

    std::string AsDebugString() const final;

private:
    gc_oop_t literal;
    int numberOfArguments;
};
