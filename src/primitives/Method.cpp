
#include "Method.h"

#include <primitivesCore/Routine.h>

#include <vmobjects/VMClass.h>
#include <vmobjects/VMMethod.h>

_Method::_Method() : PrimitiveContainer() {
    SetPrimitive("signature", new Routine<_Method>(this, &_Method::Signature));
    SetPrimitive("holder",    new Routine<_Method>(this, &_Method::Holder));
}

void _Method::Holder(pVMObject, pVMFrame frame) {
    pVMMethod self = static_cast<pVMMethod>(frame->Pop());
    frame->Push(self->GetHolder());
}

void _Method::Signature(pVMObject, pVMFrame frame) {
    pVMMethod self = static_cast<pVMMethod>(frame->Pop());
    frame->Push(self->GetSignature());
}
