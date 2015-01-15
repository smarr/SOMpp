
#include "Method.h"

#include <primitivesCore/Routine.h>

#include <vmobjects/VMClass.h>
#include <vmobjects/VMMethod.h>

_Method::_Method() : PrimitiveContainer() {
    SetPrimitive("signature", new Routine<_Method>(this, &_Method::Signature));
    SetPrimitive("holder",    new Routine<_Method>(this, &_Method::Holder));
}

void _Method::Holder(VMObject*, VMFrame* frame) {
    VMMethod* self = static_cast<VMMethod*>(frame->Pop());
    frame->Push(self->GetHolder());
}

void _Method::Signature(VMObject*, VMFrame* frame) {
    VMMethod* self = static_cast<VMMethod*>(frame->Pop());
    frame->Push(self->GetSignature());
}
