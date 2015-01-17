//
//  Delay.cpp
//  SOM
//
//  Created by Jeroen De Geeter on 10/11/13.
//
//

#include "Delay.h"

#include <primitivesCore/Routine.h>
#include <vmobjects/VMThread.h>

#include <vmobjects/ObjectFormats.h>
#include <vmobjects/VMObject.h>
#include <vmobjects/VMInteger.h>

#include <unistd.h>
#include <stdio.h>


void _Delay::Wait(Interpreter*, VMFrame* frame) {
    VMObject* self = static_cast<VMObject*>(frame->Pop());
    int64_t delay = INT_VAL(self->GetField(0));

    usleep((useconds_t) delay * 1000);

    frame->Push(self);
}

_Delay::_Delay() : PrimitiveContainer() {
    SetPrimitive("wait", new Routine<_Delay>(this, &_Delay::Wait, false));
}
