//
//  Mutex.cpp
//  SOM
//
//  Created by Jeroen De Geeter on 10/11/13.
//
//

#include "Mutex.h"

#include <vmobjects/VMMutex.h>
#include <primitivesCore/Routine.h>

void _Mutex::Lock(Interpreter*, VMFrame* frame) {
    VMMutex* mutex = static_cast<VMMutex*>(frame->GetStackElement(0));
    mutex->Lock();
}

void _Mutex::Unlock(Interpreter*, VMFrame* frame) {
    VMMutex* mutex = static_cast<VMMutex*>(frame->GetStackElement(0));
    mutex->Unlock();
}

void _Mutex::IsLocked(Interpreter*, VMFrame* frame) {
    VMMutex* mutex = static_cast<VMMutex*>(frame->Pop());

    frame->Push(load_ptr(mutex->IsLocked() ? trueObject : falseObject));
}

void _Mutex::New(Interpreter* interp, VMFrame* frame){
    frame->Pop();
    VMMutex* mutex = GetUniverse()->NewMutex();
    frame->Push(mutex);
}

_Mutex::_Mutex() : PrimitiveContainer() {
    SetPrimitive("lock",     new Routine<_Mutex>(this, &_Mutex::Lock,     false));
    SetPrimitive("unlock",   new Routine<_Mutex>(this, &_Mutex::Unlock,   false));
    SetPrimitive("isLocked", new Routine<_Mutex>(this, &_Mutex::IsLocked, false));
    SetPrimitive("new",      new Routine<_Mutex>(this, &_Mutex::New,      true));
}
