#pragma once

#include "../vmobjects/ObjectFormats.h"

using FramePrimitiveRoutine = void(VMFrame*);
using UnaryPrimitiveRoutine = vm_oop_t(vm_oop_t);
using BinaryPrimitiveRoutine = vm_oop_t(vm_oop_t, vm_oop_t);
using TernaryPrimitiveRoutine = vm_oop_t(vm_oop_t, vm_oop_t, vm_oop_t);

class Prim {
public:
    explicit Prim(bool classSide) : isClassSide(classSide) {}
    bool isClassSide;
    /* Hash is determined for each method so the correct primitive can be
     * assigned */
    size_t hash = 0;
};

class FramePrim : public Prim {
public:
    FramePrim(FramePrimitiveRoutine ptr, bool classSide)
        : Prim(classSide), pointer(ptr) {}
    explicit FramePrim() : Prim(false), pointer(nullptr) {}

    [[nodiscard]] bool IsValid() const { return pointer != nullptr; }
    void MarkObjectAsInvalid() { pointer = nullptr; }

    void (*pointer)(VMFrame*);
};

class UnaryPrim : public Prim {
public:
    UnaryPrim(UnaryPrimitiveRoutine ptr, bool classSide)
        : Prim(classSide), pointer(ptr) {}
    explicit UnaryPrim() : Prim(false), pointer(nullptr) {}

    [[nodiscard]] bool IsValid() const { return pointer != nullptr; }
    void MarkObjectAsInvalid() { pointer = nullptr; }

    vm_oop_t (*pointer)(vm_oop_t);
};

class BinaryPrim : public Prim {
public:
    BinaryPrim(BinaryPrimitiveRoutine ptr, bool classSide)
        : Prim(classSide), pointer(ptr) {}
    explicit BinaryPrim() : Prim(false), pointer(nullptr) {}

    [[nodiscard]] bool IsValid() const { return pointer != nullptr; }

    void MarkObjectAsInvalid() { pointer = nullptr; }

    vm_oop_t (*pointer)(vm_oop_t, vm_oop_t);
};

class TernaryPrim : public Prim {
public:
    TernaryPrim(TernaryPrimitiveRoutine ptr, bool classSide)
        : Prim(classSide), pointer(ptr) {}
    explicit TernaryPrim() : Prim(false), pointer(nullptr) {}

    [[nodiscard]] bool IsValid() const { return pointer != nullptr; }

    void MarkObjectAsInvalid() { pointer = nullptr; }

    vm_oop_t (*pointer)(vm_oop_t, vm_oop_t, vm_oop_t);
};
