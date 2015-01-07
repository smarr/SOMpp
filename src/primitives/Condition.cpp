#include "Condition.h"

#include <primitivesCore/Routine.h>
#include <vmobjects/VMCondition.h>

void _Condition::SignalOne(Interpreter*, VMFrame* frame) {
    VMCondition* cond = static_cast<VMCondition*>(frame->GetStackElement(0));
    cond->SignalOne();
}

void _Condition::SignalAll(Interpreter*, VMFrame* frame) {
    VMCondition* cond = static_cast<VMCondition*>(frame->GetStackElement(0));
    cond->SignalAll();
}

void _Condition::Await(Interpreter*, VMFrame* frame) {
    VMCondition* cond = static_cast<VMCondition*>(frame->GetStackElement(0));
    cond->Await();
}

void _Condition::Await_(Interpreter*, VMFrame* frame) {
    vm_oop_t      arg = frame->Pop();
    VMCondition* cond = static_cast<VMCondition*>(frame->Pop());
    
    int64_t timeoutInMilliseconds = INT_VAL(arg);
    bool no_timeout = cond->Await(timeoutInMilliseconds);
    
    frame->Push(load_ptr(no_timeout ? trueObject : falseObject));
}


_Condition::_Condition() : PrimitiveContainer() {
    SetPrimitive("signalOne", new Routine<_Condition>(this, &_Condition::SignalOne, false));
    SetPrimitive("signalAll", new Routine<_Condition>(this, &_Condition::SignalAll, false));
    
    SetPrimitive("await",     new Routine<_Condition>(this, &_Condition::Await,  false));
    SetPrimitive("await_",    new Routine<_Condition>(this, &_Condition::Await_, false));
    
}
