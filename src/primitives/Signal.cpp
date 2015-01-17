//
//  Signal.cpp
//  SOM
//
//  Created by Jeroen De Geeter on 9/11/13.
//
//

#include "Signal.h"

#include "../natives/VMSignal.h"
#include "../primitivesCore/Routine.h"

void _Signal::Wait(VMObject* object, VMFrame* frame){
    VMSignal* signal = (VMSignal*)frame->Pop();
    signal->Wait();
    frame->Push(signal);
}

void _Signal::Signal(VMObject* object, VMFrame* frame){
    VMSignal* signal = (VMSignal*)frame->Pop();
    signal->Signal();
    frame->Push(signal);
}

void _Signal::SignalAll(VMObject* object, VMFrame* frame){
    VMSignal* signal = (VMSignal*)frame->Pop();
    signal->SignalAll();
    frame->Push(signal);
}

void _Signal::New(VMObject* object, VMFrame* frame){
    frame->Pop();
    VMSignal* signal = GetUniverse()->NewSignal();
    frame->Push(signal);
}

_Signal::_Signal() : PrimitiveContainer() {
    SetPrimitive("wait",      new Routine<_Signal>(this, &_Signal::Wait,      false));
    SetPrimitive("signal",    new Routine<_Signal>(this, &_Signal::Signal,    false));
    SetPrimitive("signalAll", new Routine<_Signal>(this, &_Signal::SignalAll, false));
    SetPrimitive("new",       new Routine<_Signal>(this, &_Signal::New,       true));
}
