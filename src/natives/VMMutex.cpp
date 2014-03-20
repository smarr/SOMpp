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

pMutexId VMMutex::GetEmbeddedMutexId() {
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
