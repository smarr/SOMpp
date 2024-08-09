#include "VMSafePrimitive.h"

#include <string>

#include "../compiler/LexicalScope.h"
#include "../memory/Heap.h"
#include "../misc/defs.h"
#include "../primitivesCore/Primitives.h"
#include "../vm/Print.h"
#include "AbstractObject.h"
#include "ObjectFormats.h"
#include "VMClass.h"
#include "VMFrame.h"
#include "VMSymbol.h"

VMSafePrimitive* VMSafePrimitive::GetSafeUnary(VMSymbol* sig, UnaryPrim prim) {
    VMSafeUnaryPrimitive* p =
        new (GetHeap<HEAP_CLS>(), 0) VMSafeUnaryPrimitive(sig, prim);
    return p;
}

VMFrame* VMSafeUnaryPrimitive::Invoke(Interpreter*, VMFrame* frame) {
    vm_oop_t receiverObj = frame->Pop();

    frame->Push(prim.pointer(receiverObj));
    return nullptr;
}

VMSafePrimitive* VMSafePrimitive::GetSafeBinary(VMSymbol* sig,
                                                BinaryPrim prim) {
    VMSafeBinaryPrimitive* p =
        new (GetHeap<HEAP_CLS>(), 0) VMSafeBinaryPrimitive(sig, prim);
    return p;
}

VMFrame* VMSafeBinaryPrimitive::Invoke(Interpreter*, VMFrame* frame) {
    vm_oop_t rightObj = frame->Pop();
    vm_oop_t leftObj = frame->Pop();

    frame->Push(prim.pointer(leftObj, rightObj));
    return nullptr;
}

VMSafePrimitive* VMSafePrimitive::GetSafeTernary(VMSymbol* sig,
                                                 TernaryPrim prim) {
    VMSafeTernaryPrimitive* p =
        new (GetHeap<HEAP_CLS>(), 0) VMSafeTernaryPrimitive(sig, prim);
    return p;
}

VMFrame* VMSafeTernaryPrimitive::Invoke(Interpreter*, VMFrame* frame) {
    vm_oop_t arg2 = frame->Pop();
    vm_oop_t arg1 = frame->Pop();
    vm_oop_t self = frame->Pop();

    frame->Push(prim.pointer(self, arg1, arg2));
    return nullptr;
}

std::string VMSafePrimitive::AsDebugString() const {
    return "SafePrim(" + GetClass()->GetName()->GetStdString() + ">>#" +
           GetSignature()->GetStdString() + ")";
}

AbstractVMObject* VMSafeUnaryPrimitive::CloneForMovingGC() const {
    VMSafeUnaryPrimitive* prim =
        new (GetHeap<HEAP_CLS>(), 0 ALLOC_MATURE) VMSafeUnaryPrimitive(*this);
    return prim;
}

AbstractVMObject* VMSafeBinaryPrimitive::CloneForMovingGC() const {
    VMSafeBinaryPrimitive* prim =
        new (GetHeap<HEAP_CLS>(), 0 ALLOC_MATURE) VMSafeBinaryPrimitive(*this);
    return prim;
}

AbstractVMObject* VMSafeTernaryPrimitive::CloneForMovingGC() const {
    VMSafeTernaryPrimitive* prim =
        new (GetHeap<HEAP_CLS>(), 0 ALLOC_MATURE) VMSafeTernaryPrimitive(*this);
    return prim;
}

void VMSafePrimitive::InlineInto(MethodGenerationContext&, bool) {
    ErrorExit(
        "VMPrimitive::InlineInto is not supported, and should not be reached");
}
