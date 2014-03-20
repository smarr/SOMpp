//
//  Delay.cpp
//  SOM
//
//  Created by Jeroen De Geeter on 10/11/13.
//
//

#include "Delay.h"

#include "../primitivesCore/Routine.h"

#include <unistd.h>
#include <stdio.h>

void _Delay::Wait(pVMObject object, pVMFrame frame){
    pVMObject self = frame->Pop();
    pVMInteger integer = (pVMInteger)(static_cast<VMObject*>(self)->GetField(1)); // Am I certain of this 1?
    usleep(integer->GetEmbeddedInteger()*1000);
    frame->Push(self);
}

_Delay::_Delay() : PrimitiveContainer() {
    this->SetPrimitive("wait", new
                       Routine<_Delay>(this, &_Delay::Wait));
}
