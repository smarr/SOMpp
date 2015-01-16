/*
 * AbstractVMObject.cpp
 *
 *  Created on: 10.03.2011
 *      Author: christian
 */

#include "AbstractObject.h"

#include <vm/Universe.h>

#include "../interpreter/Interpreter.h"
#include "VMFrame.h"
#include "VMClass.h"
#include "VMInvokable.h"

intptr_t AbstractVMObject::GetHash() {
    return reinterpret_cast<intptr_t>(this);
}

void AbstractVMObject::Send(StdString selectorString, vm_oop_t* arguments, long argc) {
    VMSymbol* selector = GetUniverse()->SymbolFor(selectorString);
    VMFrame* frame = GetUniverse()->GetInterpreter()->GetFrame();
    frame->Push(this);

    for (long i = 0; i < argc; ++i) {
        frame->Push(arguments[i]);
    }

    VMClass* cl = this->GetClass();
    VMInvokable* invokable = cl->LookupInvokable(selector);
    (*invokable)(frame);
}

long AbstractVMObject::GetFieldIndex(VMSymbol* fieldName) {
    return this->GetClass()->LookupFieldIndex(fieldName);
}