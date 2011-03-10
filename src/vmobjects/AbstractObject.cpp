/*
 * AbstractVMObject.cpp
 *
 *  Created on: 10.03.2011
 *      Author: christian
 */

#include "AbstractObject.h"

#include "../vm/Universe.h"
#include "VMFrame.h"
#include "VMClass.h"
#include "VMInvokable.h"

int32_t AbstractVMObject::GetHash() {
	return (int32_t)this;
}

int32_t AbstractVMObject::GetGCField() const {
	return gcfield;
}

void AbstractVMObject::SetGCField(int32_t value) {
	gcfield = value;
}

void AbstractVMObject::Send(StdString selectorString, AbstractVMObject** arguments, int argc) {
    pVMSymbol selector = _UNIVERSE->SymbolFor(selectorString);
    pVMFrame frame = _UNIVERSE->GetInterpreter()->GetFrame();
    frame->Push(this);

    for(int i = 0; i < argc; ++i) {
        frame->Push(arguments[i]);
    }

    pVMClass cl = this->GetClass();
    pVMInvokable invokable = (pVMInvokable)(cl->LookupInvokable(selector));
    (*invokable)(frame);
}
