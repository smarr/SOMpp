
#include "Primitive.h"

#include <primitivesCore/Routine.h>

#include <vmobjects/VMClass.h>
#include <vmobjects/VMMethod.h>

_Primitive::_Primitive() : PrimitiveContainer() {
    SetPrimitive("signature", new Routine<_Primitive>(this, &_Primitive::Signature));
    SetPrimitive("holder",    new Routine<_Primitive>(this, &_Primitive::Holder));
}

void _Primitive::Holder(pVMObject, VMFrame* frame) {
    VMMethod* self = static_cast<VMMethod*>(frame->Pop());
    frame->Push(self->GetHolder());
}

void _Primitive::Signature(pVMObject, VMFrame* frame) {
    VMMethod* self = static_cast<VMMethod*>(frame->Pop());
    frame->Push(self->GetSignature());
}
