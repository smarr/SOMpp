#include "VMMutex.h"
#include <vm/Universe.h>

const long VMMutex::VMMutexNumberOfFields = 0;

void VMMutex::Lock() {
    assert(lock);
    lock->lock();
}

void VMMutex::Unlock() {
    assert(lock);
    lock->unlock();
}

bool VMMutex::IsLocked() const {
    assert(lock);
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

void VMMutex::MarkObjectAsInvalid() {
    clazz = (GCClass*) INVALID_GC_POINTER;
    std::unique_lock<recursive_mutex>** lock_for_reset = const_cast<std::unique_lock<recursive_mutex>**>(&lock);
    *lock_for_reset = nullptr;
}
