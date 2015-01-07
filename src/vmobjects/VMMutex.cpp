#include "VMMutex.h"
#include <vm/Universe.h>

const long VMMutex::VMMutexNumberOfFields = 0;

void VMMutex::Lock() {
    lock->lock();
}

void VMMutex::Unlock() {
    lock->unlock();
}

bool VMMutex::IsLocked() const {
    return lock->owns_lock();
}

VMCondition* VMMutex::NewCondition() {
    return GetUniverse()->NewCondition(this);
}


StdString VMMutex::AsDebugString() const {
    return "VMMutex";
}

VMMutex* VMMutex::Clone() const {
    VMMutex* clone = new (GetHeap<HEAP_CLS>(),
                              GetAdditionalSpaceConsumption() ALLOC_MATURE)
        VMMutex(lock);
    clone->clazz = clazz;
    return clone;
}

