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
    int err = pthread_cond_init(&embeddedSignalId, NULL);
    if(err != 0) {
        fprintf(stderr, "Could not initialise signal.\n");
    }
    pthread_mutex_init(&embeddedMutexId, NULL);
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

#if GC_TYPE==PAUSELESS
VMSignal* VMSignal::Clone(Interpreter* thread) {
    VMSignal* clone = new (_HEAP, thread, objectSize - sizeof(VMSignal)) VMSignal(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    return clone;
}
VMSignal* VMSignal::Clone(PauselessCollectorThread* thread) {
    VMSignal* clone = new (_HEAP, thread, objectSize - sizeof(VMSignal)) VMSignal(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    return clone;
}

void VMSignal::MarkReferences() {
    ReadBarrierForGCThread(&clazz);
}

void VMSignal::CheckMarking(void (*walk)(AbstractVMObject*)) {
    assert(GetNMTValue(clazz) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(clazz));
    walk(Untag(clazz));
}
#else
void VMSignal::WalkObjects(VMOBJECT_PTR (*walk)(VMOBJECT_PTR)) {
    clazz = (GCClass*) (walk(READBARRIER(clazz)));
}

VMSignal* VMSignal::Clone() {
    VMSignal* clone = new (_HEAP, _PAGE, objectSize - sizeof(VMSignal)) VMSignal(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    return clone;
}
#endif
