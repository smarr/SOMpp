#include "Thread.h"

#include <primitivesCore/Routine.h>

#include <vmobjects/VMThread.h>
#include <vmobjects/VMString.h>

void _Thread::Name(Interpreter*, VMFrame* frame) {
    VMThread* thread = static_cast<VMThread*>(frame->Pop());
    frame->Push(thread->GetName());
}

void _Thread::Name_(Interpreter*, VMFrame* frame) {
    VMString* name   = static_cast<VMString*>(frame->Pop());
    VMThread* thread = static_cast<VMThread*>(frame->GetStackElement(0));
    thread->SetName(name);
}

void _Thread::Join(Interpreter*, VMFrame* frame) {
    VMThread* thread = static_cast<VMThread*>(frame->GetStackElement(0));
    thread->Join();
}

void _Thread::Yield(Interpreter*, VMFrame* frame) {
    VMThread::Yield();
}

void _Thread::Current(Interpreter*, VMFrame* frame) {
    frame->Pop(); // pop receiver, i.e., Thread class
    frame->Push(VMThread::Current());
}


_Thread::_Thread() : PrimitiveContainer() {
    SetPrimitive("name",    new Routine<_Thread>(this, &_Thread::Name,    false));
    SetPrimitive("name_",   new Routine<_Thread>(this, &_Thread::Name_,   false));
    SetPrimitive("join",    new Routine<_Thread>(this, &_Thread::Join,    false));
    SetPrimitive("yield",   new Routine<_Thread>(this, &_Thread::Yield,   true));
    SetPrimitive("current", new Routine<_Thread>(this, &_Thread::Current, true));
}
