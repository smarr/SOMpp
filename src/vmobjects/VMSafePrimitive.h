#pragma once

#include "../primitivesCore/PrimitiveContainer.h"
#include "Signature.h"

class VMSafePrimitive : public VMInvokable {
public:
    typedef GCSafePrimitive Stored;

    VMSafePrimitive(VMSymbol* sig) : VMInvokable(sig) {}

    VMClass* GetClass() const final { return load_ptr(primitiveClass); }

    bool IsPrimitive() const final { return true; };

    void InlineInto(MethodGenerationContext& mgenc,
                    bool mergeScope = true) final;

    static VMSafePrimitive* GetSafeUnary(VMSymbol* sig, UnaryPrim prim);
    static VMSafePrimitive* GetSafeBinary(VMSymbol* sig, BinaryPrim prim);
    static VMSafePrimitive* GetSafeTernary(VMSymbol* sig, TernaryPrim prim);

    std::string AsDebugString() const final;

    inline size_t GetNumberOfArguments() const final {
        return Signature::GetNumberOfArguments(load_ptr(signature));
    }
};

class VMSafeUnaryPrimitive : public VMSafePrimitive {
public:
    typedef GCSafeUnaryPrimitive Stored;

    VMSafeUnaryPrimitive(VMSymbol* sig, UnaryPrim prim)
        : VMSafePrimitive(sig), prim(prim) {
        write_barrier(this, sig);
    }

    inline size_t GetObjectSize() const override {
        return sizeof(VMSafeUnaryPrimitive);
    }

    VMFrame* Invoke(VMFrame*) override;

    AbstractVMObject* CloneForMovingGC() const final;

    void MarkObjectAsInvalid() final {
        VMSafePrimitive::MarkObjectAsInvalid();
        prim.MarkObjectAsInvalid();
    }

    bool IsMarkedInvalid() const final { return !prim.IsValid(); }

private:
    UnaryPrim prim;
};

class VMSafeBinaryPrimitive : public VMSafePrimitive {
public:
    typedef GCSafeBinaryPrimitive Stored;

    VMSafeBinaryPrimitive(VMSymbol* sig, BinaryPrim prim)
        : VMSafePrimitive(sig), prim(prim) {
        write_barrier(this, sig);
    }

    inline size_t GetObjectSize() const override {
        return sizeof(VMSafeBinaryPrimitive);
    }

    VMFrame* Invoke(VMFrame*) override;

    AbstractVMObject* CloneForMovingGC() const final;

    void MarkObjectAsInvalid() final {
        VMSafePrimitive::MarkObjectAsInvalid();
        prim.MarkObjectAsInvalid();
    }

    bool IsMarkedInvalid() const final { return !prim.IsValid(); }

private:
    BinaryPrim prim;
};

class VMSafeTernaryPrimitive : public VMSafePrimitive {
public:
    typedef GCSafeTernaryPrimitive Stored;

    VMSafeTernaryPrimitive(VMSymbol* sig, TernaryPrim prim)
        : VMSafePrimitive(sig), prim(prim) {
        write_barrier(this, sig);
    }

    inline size_t GetObjectSize() const override {
        return sizeof(VMSafeTernaryPrimitive);
    }

    VMFrame* Invoke(VMFrame*) override;

    AbstractVMObject* CloneForMovingGC() const final;

    void MarkObjectAsInvalid() final {
        VMSafePrimitive::MarkObjectAsInvalid();
        prim.MarkObjectAsInvalid();
    }

    bool IsMarkedInvalid() const final { return !prim.IsValid(); }

private:
    TernaryPrim prim;
};
