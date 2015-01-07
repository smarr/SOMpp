#include "VMCondition.h"

#include <chrono>

const long VMCondition::VMConditionNumberOfFields = 0;

void VMCondition::SignalOne() {
    cond_var->notify_one();
}

void VMCondition::SignalAll() {
    cond_var->notify_all();
}


void VMCondition::Await() {
    cond_var->wait(*lock);
}

bool VMCondition::Await(int64_t timeoutMilliseconds) {
    return cv_status::no_timeout == cond_var->wait_for(
                    *lock, std::chrono::milliseconds(timeoutMilliseconds));
}

StdString VMCondition::AsDebugString() const {
    return "VMCondition";
}

VMCondition* VMCondition::Clone() const {
    VMCondition* clone = new (GetHeap<HEAP_CLS>(),
                              GetAdditionalSpaceConsumption() ALLOC_MATURE)
                            VMCondition(lock, cond_var);
    clone->clazz = clazz;
    return clone;
}
