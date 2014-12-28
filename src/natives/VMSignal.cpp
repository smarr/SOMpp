//
//  VMSignal.cpp
//  SOM
//
//  Created by Jeroen De Geeter on 5/03/14.
//
//

#include "VMSignal.h"

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
pVMSignal VMSignal::Clone(Interpreter* thread) {
    pVMSignal clone = new (_HEAP, thread, objectSize - sizeof(VMSignal)) VMSignal(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    return clone;
}
pVMSignal VMSignal::Clone(PauselessCollectorThread* thread) {
    pVMSignal clone = new (_HEAP, thread, objectSize - sizeof(VMSignal)) VMSignal(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    return clone;
}
#endif
