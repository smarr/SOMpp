//
//  VMMutex.cpp
//  SOM
//
//  Created by Jeroen De Geeter on 5/03/14.
//
//

#include "VMMutex.h"

const int VMMutex::VMMutexNumberOfFields = 0;

VMMutex::VMMutex() : VMObject(VMMutexNumberOfFields) {
    int err = pthread_mutex_init(&embeddedMutexId, NULL);
    if(err != 0) {
        fprintf(stderr, "Could not initialise mutex.\n");
    }
}

pthread_mutex_t* VMMutex::GetEmbeddedMutexId() {
    return &embeddedMutexId;
}


void VMMutex::Lock() {
    pthread_mutex_lock(&embeddedMutexId);
}


void VMMutex::Unlock() {
    pthread_mutex_unlock(&embeddedMutexId);
}


bool VMMutex::IsLocked() {
    int res = pthread_mutex_trylock(&embeddedMutexId);
    if (res == 0) {
        pthread_mutex_unlock(&embeddedMutexId);
        return true;
    } else {
        return false;
    }
}

#if GC_TYPE==PAUSELESS
pVMMutex VMMutex::Clone(Interpreter* thread) {
    pVMMutex clone = new (_HEAP, thread, objectSize - sizeof(VMMutex)) VMMutex(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    return clone;
}
pVMMutex VMMutex::Clone(PauselessCollectorThread* thread) {
    pVMMutex clone = new (_HEAP, thread, objectSize - sizeof(VMMutex)) VMMutex(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    return clone;
}
#endif
