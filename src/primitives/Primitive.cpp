
#include "Primitive.h"

#include <primitivesCore/Routine.h>

#include <vmobjects/VMClass.h>
#include <vmobjects/VMMethod.h>

_Primitive::_Primitive() : PrimitiveContainer() {
    SetPrimitive("signature", new Routine<_Primitive>(this, &_Primitive::Signature, false));
    SetPrimitive("holder",    new Routine<_Primitive>(this, &_Primitive::Holder,    false));
}

void _Primitive::Holder(Interpreter*, VMFrame* frame) {
    VMMethod* self = static_cast<VMMethod*>(frame->Pop());
    frame->Push(self->GetHolder());
}

void _Primitive::Signature(Interpreter*, VMFrame* frame) {
    VMMethod* self = static_cast<VMMethod*>(frame->Pop());
    frame->Push(self->GetSignature());
}
