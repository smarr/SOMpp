#include "Method.h"

#include <cstddef>

#include "../primitivesCore/PrimitiveContainer.h"
#include "../primitivesCore/Routine.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMArray.h"
#include "../vmobjects/VMClass.h"  // NOLINT(misc-include-cleaner) it's required to make the types complete
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMSymbol.h"  // NOLINT(misc-include-cleaner) it's required to make the types complete

static vm_oop_t mHolder(vm_oop_t rcvr) {
    VMMethod* self = static_cast<VMMethod*>(rcvr);
    return self->GetHolder();
}

static vm_oop_t mSignature(vm_oop_t rcvr) {
    VMMethod* self = static_cast<VMMethod*>(rcvr);
    return self->GetSignature();
}

void _Method::InvokeOn_With_(Interpreter* interp, VMFrame* frame) {
    // REM: this is a clone with _Primitive::InvokeOn_With_
    VMArray* args = static_cast<VMArray*>(frame->Pop());
    vm_oop_t rcvr = static_cast<vm_oop_t>(frame->Pop());
    VMMethod* mthd = static_cast<VMMethod*>(frame->Pop());

    frame->Push(rcvr);

    size_t num_args = args->GetNumberOfIndexableFields();
    for (size_t i = 0; i < num_args; i++) {
        vm_oop_t arg = args->GetIndexableField(i);
        frame->Push(arg);
    }
    mthd->Invoke(interp, frame);
}

_Method::_Method() : PrimitiveContainer() {
    Add("signature", &mSignature, false);
    Add("holder", &mHolder, false);
    SetPrimitive("invokeOn_with_",
                 new Routine<_Method>(this, &_Method::InvokeOn_With_, false));
}
