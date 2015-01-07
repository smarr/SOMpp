#pragma once

#include "VMObject.h"

#include <condition_variable>

class VMCondition : public VMObject {
public:
    typedef GCCondition Stored;
    
    VMCondition(std::unique_lock<recursive_mutex>* const lock)
        : VMObject(VMConditionNumberOfFields),
          lock(lock), cond_var(new condition_variable_any()) {};
    
    void SignalOne();
    void SignalAll();
    
    void Await();
    bool Await(int64_t timeoutMilliseconds);
    
    virtual StdString AsDebugString() const;
    virtual VMCondition* Clone() const;
    
private:
    VMCondition(std::unique_lock<recursive_mutex>* const lock,
                std::condition_variable_any* const cond_var)
        : VMObject(VMConditionNumberOfFields),
          lock(lock), cond_var(cond_var) {};
    
    std::unique_lock<recursive_mutex>* const lock;
    std::condition_variable_any* const cond_var;
    
    static const long VMConditionNumberOfFields;
    
};
