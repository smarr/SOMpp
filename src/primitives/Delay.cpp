//
//  Delay.cpp
//  SOM
//
//  Created by Jeroen De Geeter on 10/11/13.
//
//

#include "Delay.h"

#include <primitivesCore/Routine.h>
#include <natives/VMThread.h>

#include <vmobjects/ObjectFormats.h>

#include <unistd.h>
#include <stdio.h>

void _Delay::Wait(pVMObject object, pVMFrame frame){
    pVMObject self = frame->Pop();
    pVMInteger integer = (pVMInteger)(static_cast<VMObject*>(self)->GetField(0));
    //cout << "DELAY : thread sleeping, threadID: " << _UNIVERSE->GetInterpreter()->GetThread()->GetThreadId() << "needs to wait: " << integer->GetEmbeddedInteger()*1000 << endl;
    usleep(integer->GetEmbeddedInteger()*1000);
    //cout << "DELAY : thread has woken up, threadID: " << _UNIVERSE->GetInterpreter()->GetThread()->GetThreadId() << endl;
    frame->Push(self);
}

_Delay::_Delay() : PrimitiveContainer() {
    this->SetPrimitive("wait", new
                       Routine<_Delay>(this, &_Delay::Wait));
}
