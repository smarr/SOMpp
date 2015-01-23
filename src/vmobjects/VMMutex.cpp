#include "VMMutex.h"
#include "VMClass.h"
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

VMCondition* VMMutex::NewCondition(Page* page) {
    return GetUniverse()->NewCondition(this, page);
}


StdString VMMutex::AsDebugString() const {
    return "VMMutex";
}

VMMutex* VMMutex::Clone(Page* page) const {
    VMMutex* clone = new (page, 0 ALLOC_MATURE) VMMutex(lock);
    clone->clazz = clazz;
    return clone;
}

void VMMutex::MarkObjectAsInvalid() {
    clazz = (GCClass*) INVALID_GC_POINTER;
    std::unique_lock<recursive_mutex>** lock_for_reset = const_cast<std::unique_lock<recursive_mutex>**>(&lock);
    *lock_for_reset = nullptr;
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
#endif
