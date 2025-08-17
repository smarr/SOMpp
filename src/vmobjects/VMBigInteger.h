#pragma once

#include "../lib/InfInt.h"
#include "../misc/defs.h"
#include "AbstractObject.h"

class VMBigInteger : public AbstractVMObject {
public:
    typedef GCBigInteger Stored;

    explicit VMBigInteger(const char* value, bool negate)
        : embeddedInteger(negate ? -InfInt(value) : InfInt(value)) {}
    explicit VMBigInteger(int64_t value) : embeddedInteger(InfInt(value)) {}
    explicit VMBigInteger(const InfInt&& value) : embeddedInteger(value) {}

    ~VMBigInteger() override = default;

    [[nodiscard]] inline const InfInt* GetEmbeddedInteger() const {
        return &embeddedInteger;
    }

    [[nodiscard]] VMBigInteger* CloneForMovingGC() const override;
    [[nodiscard]] VMClass* GetClass() const override;

    [[nodiscard]] inline size_t GetObjectSize() const override {
        return sizeof(VMBigInteger);
    }

    [[nodiscard]] inline int64_t GetHash() const override {
        return embeddedInteger.toInt64();
    }

    void MarkObjectAsInvalid() override;
    [[nodiscard]] bool IsMarkedInvalid() const override;

    [[nodiscard]] std::string AsDebugString() const override;

    /* primitive operations */
    [[nodiscard]] vm_oop_t Add(int64_t /*value*/) const;
    vm_oop_t Add(vm_oop_t /*value*/) const;

    [[nodiscard]] vm_oop_t SubtractFrom(int64_t /*value*/) const;
    vm_oop_t Subtract(vm_oop_t /*value*/) const;

    [[nodiscard]] vm_oop_t Multiply(int64_t /*value*/) const;
    vm_oop_t Multiply(vm_oop_t /*value*/) const;

    [[nodiscard]] vm_oop_t DivisionFrom(int64_t /*value*/) const;
    vm_oop_t DivideBy(vm_oop_t /*value*/) const;

    [[nodiscard]] vm_oop_t ModuloFrom(int64_t /*value*/) const;
    vm_oop_t Modulo(vm_oop_t /*value*/) const;

    [[nodiscard]] vm_oop_t Negate();

    vm_oop_t IsEqual(VMBigInteger* /*o*/) const;

    /* fields */
    make_testable(public);

    InfInt embeddedInteger;
};
