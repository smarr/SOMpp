/*
 * AbstractVMObject.cpp
 *
 *  Created on: 10.03.2011
 *      Author: christian
 */

#include "AbstractObject.h"

#include <string>

#include "../interpreter/Interpreter.h"
#include "../vm/Symbols.h"
#include "../vmobjects/ObjectFormats.h"
#include "VMClass.h"
#include "VMFrame.h"
#include "VMInvokable.h"
#include "VMSymbol.h"

void AbstractVMObject::Send(std::string selectorString, vm_oop_t* arguments,
                            long argc) {
    VMFrame* frame = Interpreter::GetFrame();
    VMSymbol* selector = SymbolFor(selectorString);
    frame->Push(this);

    for (long i = 0; i < argc; ++i) {
        frame->Push(arguments[i]);
    }

    VMClass* cl = GetClass();
    VMInvokable* invokable = cl->LookupInvokable(selector);
    invokable->Invoke(frame);
}

long AbstractVMObject::GetFieldIndex(VMSymbol* fieldName) const {
    return GetClass()->LookupFieldIndex(fieldName);
}
