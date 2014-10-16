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

long AbstractVMObject::GetFieldIndex(pVMSymbol fieldName) {
    return this->GetClass()->LookupFieldIndex(fieldName);
}

AbstractVMObject* AbstractVMObject::ProtectedClone(Page* page) {;
    if (pthread_mutex_lock(&beingMovedMutex) == 0) {
        newClone = Clone(page);
        pthread_cond_signal(&beingMovedCondition);
        pthread_mutex_unlock(&beingMovedMutex);
    } else {
        while (!newClone) {
            pthread_cond_wait(&beingMovedCondition, &beingMovedMutex);
        }
    }
    return newClone;
}