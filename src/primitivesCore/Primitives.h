#pragma once

#include "../vmobjects/ObjectFormats.h"

using UnaryPrimitiveRoutine = vm_oop_t(vm_oop_t);
using BinaryPrimitiveRoutine = vm_oop_t(vm_oop_t, vm_oop_t);

class UnaryPrim {
public:
    UnaryPrim(UnaryPrimitiveRoutine ptr, bool classSide)
        : pointer(ptr), isClassSide(classSide) {}
    explicit UnaryPrim() : pointer(nullptr), isClassSide(false) {}

    bool IsValid() const { return pointer != nullptr; }

    void MarkObjectAsInvalid() { pointer = nullptr; }

    vm_oop_t (*pointer)(vm_oop_t);

    bool isClassSide;
};

class BinaryPrim {
public:
    BinaryPrim(BinaryPrimitiveRoutine ptr, bool classSide)
        : pointer(ptr), isClassSide(classSide) {}
    explicit BinaryPrim() : pointer(nullptr), isClassSide(false) {}

    bool IsValid() const { return pointer != nullptr; }

    void MarkObjectAsInvalid() { pointer = nullptr; }

    vm_oop_t (*pointer)(vm_oop_t, vm_oop_t);

    bool isClassSide;
};
