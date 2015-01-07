#pragma once

#include <mutex>

#include "VMObject.h"

class VMMutex : public VMObject {
public:
    typedef GCMutex Stored;
    
    VMMutex() : VMObject(VMMutexNumberOfFields),
                lock(new unique_lock<recursive_mutex>(
                                *new recursive_mutex(), std::defer_lock)) {};
    
    void Lock();
    void Unlock();
    bool IsLocked() const;
    
    VMCondition* NewCondition();
    
    virtual StdString AsDebugString() const;
    virtual VMMutex* Clone() const;
    virtual void MarkObjectAsInvalid();
    
    std::unique_lock<recursive_mutex>* GetLock() const { return lock; }
    
private:
    VMMutex(std::unique_lock<recursive_mutex>* const lock)
        : VMObject(VMMutexNumberOfFields), lock(lock) {};
    std::unique_lock<recursive_mutex>* const lock;
    
    static const long VMMutexNumberOfFields;
    
};
