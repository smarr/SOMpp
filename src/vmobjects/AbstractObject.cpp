/*
 * AbstractVMObject.cpp
 *
 *  Created on: 10.03.2011
 *      Author: christian
 */

#include "AbstractObject.h"

#include "../vm/Universe.h"
#ifdef USE_TAGGING
#include "VMIntPointer.h"
#endif
#include "VMFrame.h"
#include "VMClass.h"
#include "VMInvokable.h"

size_t AbstractVMObject::GetHash() {
	return (size_t)this;
}

void AbstractVMObject::Send(StdString selectorString, pVMObject* arguments, int argc) {
    pVMSymbol selector = _UNIVERSE->SymbolFor(selectorString);
    pVMFrame frame = _UNIVERSE->GetInterpreter()->GetFrame();
    frame->Push(this);

    for(int i = 0; i < argc; ++i) {
        frame->Push(arguments[i]);
    }

    pVMClass cl = this->GetClass();
    pVMInvokable invokable = static_cast<pVMInvokable>(cl->LookupInvokable(selector));
    (*invokable)(frame);
}

int AbstractVMObject::GetFieldIndex(pVMSymbol fieldName) const {
	return this->GetClass()->LookupFieldIndex(fieldName);
}
pVMObject AbstractVMObject::GetField(int index) const {
	//we have to emulate field 0 = class
	if (index==0)
		return this->GetClass();
}
