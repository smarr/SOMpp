//
//  VMMutex.cpp
//  SOM
//
//  Created by Jeroen De Geeter on 5/03/14.
//
//

#include "VMMutex.h"
#include <vmObjects/VMClass.h>

#include <interpreter/Interpreter.h>

const int VMMutex::VMMutexNumberOfFields = 0;

VMMutex::VMMutex() : VMObject(VMMutexNumberOfFields) {
    int err = pthread_mutex_init(&embeddedMutexId, nullptr);
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

VMMutex* VMMutex::Clone(Page* page) {
    VMMutex* clone = new (page, objectSize - sizeof(VMMutex)) VMMutex(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    return clone;
}

#if GC_TYPE==PAUSELESS
void VMMutex::MarkReferences() {
    ReadBarrierForGCThread(&clazz);
}

void VMMutex::CheckMarking(void (*walk)(vm_oop_t)) {
    assert(GetNMTValue(clazz) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(clazz));
    walk(Untag(clazz));
}

#else
void VMMutex::WalkObjects(walk_heap_fn walk) {
    clazz = (GCClass*) (walk(clazz));
}
#endif
