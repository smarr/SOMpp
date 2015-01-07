#include "Mutex.h"

#include <primitivesCore/Routine.h>

#include <vm/Universe.h>

#include <vmobjects/VMMutex.h>
#include <vmobjects/VMCondition.h>

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

void _Mutex::NewCondition(Interpreter*, VMFrame* frame) {
    VMMutex* mutex = static_cast<VMMutex*>(frame->Pop());
    frame->Push(mutex->NewCondition());
}

void _Mutex::New(Interpreter*, VMFrame* frame) {
    frame->Pop(); // pop receiver, i.e., Mutex class
    frame->Push(GetUniverse()->NewMutex());
}

_Mutex::_Mutex() : PrimitiveContainer() {
    SetPrimitive("lock",         new Routine<_Mutex>(this, &_Mutex::Lock,         false));
    SetPrimitive("unlock",       new Routine<_Mutex>(this, &_Mutex::Unlock,       false));
    SetPrimitive("isLocked",     new Routine<_Mutex>(this, &_Mutex::IsLocked,     false));
    SetPrimitive("newCondition", new Routine<_Mutex>(this, &_Mutex::NewCondition, false));
    SetPrimitive("new",          new Routine<_Mutex>(this, &_Mutex::New,          true));
}
