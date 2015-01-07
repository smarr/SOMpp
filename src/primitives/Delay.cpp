#include "Delay.h"

#include <primitivesCore/Routine.h>
#include <vm/Universe.h>
#include <vmobjects/VMObject.h>

#include <chrono>
#include <thread>

void _Delay::Wait(Interpreter*, VMFrame* frame) {
    VMObject* mutex = static_cast<VMObject*>(frame->GetStackElement(0));
    
    std::chrono::milliseconds durationInMilliseconds(INT_VAL(mutex->GetField(0)));
    std::this_thread::sleep_for(durationInMilliseconds);
}

_Delay::_Delay() : PrimitiveContainer() {
    SetPrimitive("wait",         new Routine<_Delay>(this, &_Delay::Wait, false));
}
