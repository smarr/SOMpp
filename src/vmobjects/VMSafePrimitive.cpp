#include "VMSafePrimitive.h"

#include <string>

#include "../memory/Heap.h"
#include "../misc/defs.h"
#include "../primitivesCore/Primitives.h"
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

void VMSafeUnaryPrimitive::Invoke(Interpreter*, VMFrame* frame) {
    vm_oop_t receiverObj = frame->Pop();

    frame->Push(prim.pointer(receiverObj));
}

VMSafePrimitive* VMSafePrimitive::GetSafeBinary(VMSymbol* sig,
                                                BinaryPrim prim) {
    VMSafeBinaryPrimitive* p =
        new (GetHeap<HEAP_CLS>(), 0) VMSafeBinaryPrimitive(sig, prim);
    return p;
}

void VMSafeBinaryPrimitive::Invoke(Interpreter*, VMFrame* frame) {
    vm_oop_t rightObj = frame->Pop();
    vm_oop_t leftObj = frame->Pop();

    frame->Push(prim.pointer(leftObj, rightObj));
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
