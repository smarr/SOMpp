//
//  VMSignal.cpp
//  SOM
//
//  Created by Jeroen De Geeter on 5/03/14.
//
//

#include "VMSignal.h"
#include <vmObjects/VMClass.h>

#include <interpreter/Interpreter.h>

const int VMSignal::VMSignalNumberOfFields = 0;

VMSignal::VMSignal() : VMObject(VMSignalNumberOfFields) {
    int err = pthread_cond_init(&embeddedSignalId, nullptr);
    if(err != 0) {
        fprintf(stderr, "Could not initialise signal.\n");
    }
    pthread_mutex_init(&embeddedMutexId, nullptr);
}

pthread_cond_t* VMSignal::GetEmbeddedSignalId() {
    return &embeddedSignalId;
}

pthread_mutex_t* VMSignal::GetEmbeddedMutexId() {
    return &embeddedMutexId;
}

void VMSignal::Wait() {
    pthread_mutex_lock(&embeddedMutexId);
    pthread_cond_wait(&embeddedSignalId, &embeddedMutexId);
    pthread_mutex_unlock(&embeddedMutexId);
}

void VMSignal::Signal() {
    pthread_mutex_lock(&embeddedMutexId);
    pthread_cond_signal(&embeddedSignalId);
    pthread_mutex_unlock(&embeddedMutexId);
}


bool VMSignal::SignalAll() {
    return pthread_cond_broadcast(&embeddedSignalId);
}

VMSignal* VMSignal::Clone(Page* page) {
    VMSignal* clone = new (page, objectSize - sizeof(VMSignal)) VMSignal(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    return clone;
}

#if GC_TYPE==PAUSELESS
void VMSignal::MarkReferences() {
    ReadBarrierForGCThread(&clazz);
}

void VMSignal::CheckMarking(void (*walk)(vm_oop_t)) {
    assert(GetNMTValue(clazz) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(clazz));
    walk(Untag(clazz));
}
#endif
