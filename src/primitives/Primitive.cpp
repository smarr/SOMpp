
#include "Primitive.h"

#include <primitivesCore/Routine.h>

#include <vmobjects/VMClass.h>
#include <vmobjects/VMMethod.h>

_Primitive::_Primitive() : PrimitiveContainer() {
    SetPrimitive("signature", new Routine<_Primitive>(this, &_Primitive::Signature,           false));
    SetPrimitive("holder",    new Routine<_Primitive>(this, &_Primitive::Holder,              false));
    SetPrimitive("invokeOn_with_", new Routine<_Primitive>(this, &_Primitive::InvokeOn_With_, false));
}

void _Primitive::Holder(Interpreter*, VMFrame* frame) {
    VMMethod* self = static_cast<VMMethod*>(frame->Pop());
    frame->Push(self->GetHolder());
}

void _Primitive::Signature(Interpreter*, VMFrame* frame) {
    VMMethod* self = static_cast<VMMethod*>(frame->Pop());
    frame->Push(self->GetSignature());
}

void _Primitive::InvokeOn_With_(Interpreter* interp, VMFrame* frame) {
    // REM: this is a clone with _Primitive::InvokeOn_With_
    VMArray* args  = static_cast<VMArray*>(frame->Pop());
    vm_oop_t rcvr  = static_cast<vm_oop_t>(frame->Pop());
    VMMethod* mthd = static_cast<VMMethod*>(frame->Pop());
    
    
    frame->Push(rcvr);
    
    size_t num_args = args->GetNumberOfIndexableFields();
    for (size_t i = 0; i < num_args; i++) {
        vm_oop_t arg = args->GetIndexableField(i);
        frame->Push(arg);
    }
    mthd->Invoke(interp, frame);
}
