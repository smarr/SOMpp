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

void _Thread::Join(pVMObject object, pVMFrame frame){
    pVMThread thread = (pVMThread)frame->Pop();
    int returnValue;
    ThreadId threadid = thread->GetEmbeddedThreadId();
    // This way we signal the gc thread that even if this is waiting to perform a join
    // the garbage collection is safe to proceed for this thread
    _HEAP->IncrementWaitingForGCThreads();
    thread->Join(&returnValue);
    _HEAP->DecrementWaitingForGCThreads();
    frame->Push(thread);
}

void _Thread::Priority_(pVMObject object, pVMFrame frame){
    pVMInteger arg = (pVMInteger)frame->Pop();
    int prio = arg->GetEmbeddedInteger();
    pVMThread thread = (pVMThread)frame->Pop();
    ThreadId threadId = thread->GetEmbeddedThreadId();
    
    int policy;
    struct sched_param param;
    pthread_getschedparam(threadId, &policy, &param);
    param.sched_priority = prio;
    pthread_setschedparam(threadId, policy, &param);
    
    frame->Push(thread);
}

// Thread class >> #yield
// |-|-|-| Thread class | <- top of stack
void _Thread::Yield(pVMObject object, pVMFrame frame) {
    VMThread::Yield();
}

// Thread class >> #current
// assert (threadClass != Interpreter_get_thread());
// assert (Interpreter_get_thread()->GetClass() == threadClass);
void _Thread::Current(pVMObject object, pVMFrame frame) {
    pVMClass threadClass = (pVMClass)frame->Pop();
    frame->Push(_UNIVERSE->GetInterpreter()->GetThread());
}

 
_Thread::_Thread() : PrimitiveContainer() {
    this->SetPrimitive("join", new
                       Routine<_Thread>(this, &_Thread::Join));
    
    this->SetPrimitive("priority_", new
                       Routine<_Thread>(this, &_Thread::Priority_));

    this->SetPrimitive("yield", new
                       Routine<_Thread>(this, &_Thread::Yield));

    this->SetPrimitive("current", new
                       Routine<_Thread>(this, &_Thread::Current));
}
