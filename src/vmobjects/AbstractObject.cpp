/*
 * AbstractVMObject.cpp
 *
 *  Created on: 10.03.2011
 *      Author: christian
 */

#include "AbstractObject.h"

#include <vm/Universe.h>

#include "VMFrame.h"
#include "VMClass.h"
#include "VMInvokable.h"

size_t AbstractVMObject::GetHash() {
    return (size_t) this;
}

void AbstractVMObject::Send(StdString selectorString, pVMObject* arguments, long argc) {
    pVMSymbol selector = _UNIVERSE->SymbolFor(selectorString);
    pVMFrame frame = _UNIVERSE->GetInterpreter()->GetFrame();
    frame->Push(this);

    for (long i = 0; i < argc; ++i) {
        frame->Push(arguments[i]);
    }

    pVMClass cl = this->GetClass();
    pVMInvokable invokable = cl->LookupInvokable(selector);
    (*invokable)(frame);
}

long AbstractVMObject::GetFieldIndex(pVMSymbol fieldName) const {
    return this->GetClass()->LookupFieldIndex(fieldName);
}
pVMObject AbstractVMObject::GetField(long index) const {
    // we have to emulate field 0 = class
    if (index == 0)
        return this->GetClass();
}
