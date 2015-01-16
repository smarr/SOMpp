//
//  Thread.cpp
//  SOM
//
//  Created by Jeroen De Geeter on 31/10/13.
//
//

#include "Thread.h"

#include "../natives/VMThread.h"
#include "../primitivesCore/Routine.h"
#include "../interpreter/Interpreter.h"

void _Thread::Join(VMObject* object, VMFrame* frame){
    VMThread* thread = (VMThread*)frame->Pop();
    int returnValue;
    pthread_t threadid = thread->GetEmbeddedThreadId();

#if GC_TYPE==PAUSELESS
    GetUniverse()->GetInterpreter()->EnableBlocked();
#else
    _HEAP->IncrementWaitingForGCThreads();
#endif
    
    thread->Join(&returnValue);
    
#if GC_TYPE==PAUSELESS
    GetUniverse()->GetInterpreter()->DisableBlocked();
#else
    _HEAP->DecrementWaitingForGCThreads();
#endif
    
    frame->Push(thread);
}

void _Thread::Priority_(VMObject* object, VMFrame* frame){
    VMInteger* arg = (VMInteger*)frame->Pop();
    int prio = arg->GetEmbeddedInteger();
    VMThread* thread = (VMThread*)frame->Pop();
    pthread_t threadId = thread->GetEmbeddedThreadId();
    
    int policy;
    struct sched_param param;
    pthread_getschedparam(threadId, &policy, &param);
    param.sched_priority = prio;
    pthread_setschedparam(threadId, policy, &param);
    
    frame->Push(thread);
}

// Thread class >> #yield
// |-|-|-| Thread class | <- top of stack
void _Thread::Yield(VMObject* object, VMFrame* frame) {
    VMThread::Yield();
}

// Thread class >> #current
// assert (threadClass != Interpreter_get_thread());
// assert (Interpreter_get_thread()->GetClass() == threadClass);
void _Thread::Current(VMObject* object, VMFrame* frame) {
    VMClass* threadClass = (VMClass*)frame->Pop();
    frame->Push(GetUniverse()->GetInterpreter()->GetThread());
}

_Thread::_Thread() : PrimitiveContainer() {
    SetPrimitive("join",      new Routine<_Thread>(this, &_Thread::Join));
    SetPrimitive("priority_", new Routine<_Thread>(this, &_Thread::Priority_));
    SetPrimitive("yield",     new Routine<_Thread>(this, &_Thread::Yield));
    SetPrimitive("current",   new Routine<_Thread>(this, &_Thread::Current));
}
