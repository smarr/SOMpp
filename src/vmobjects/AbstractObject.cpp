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

void AbstractVMObject::Send(StdString selectorString, oop_t* arguments, long argc) {
    VMSymbol* selector = GetUniverse()->SymbolFor(selectorString);
    VMFrame* frame = GetUniverse()->GetInterpreter()->GetFrame();
    frame->Push(this);

    for (long i = 0; i < argc; ++i) {
        frame->Push(arguments[i]);
    }

    VMClass* cl = GetClass();
    VMInvokable* invokable = cl->LookupInvokable(selector);
    (*invokable)(frame);
}

long AbstractVMObject::GetFieldIndex(VMSymbol* fieldName) const {
    return GetClass()->LookupFieldIndex(fieldName);
}
