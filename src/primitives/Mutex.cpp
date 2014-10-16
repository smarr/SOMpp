//
//  Mutex.cpp
//  SOM
//
//  Created by Jeroen De Geeter on 10/11/13.
//
//

#include "Mutex.h"

#include "../natives/VMMutex.h"
#include "../primitivesCore/Routine.h"

void _Mutex::Lock(pVMObject object, pVMFrame frame){
    pVMMutex mutex = (pVMMutex)frame->Pop();
    mutex->Lock();
    frame->Push(mutex);
}

void _Mutex::Unlock(pVMObject object, pVMFrame frame){
    pVMMutex mutex = (pVMMutex)frame->Pop();
    mutex->Unlock();
    frame->Push(mutex);
}

void _Mutex::IsLocked(pVMObject object, pVMFrame frame){
    pVMMutex mutex = (pVMMutex)frame->Pop();
    if (mutex->IsLocked()) {
        frame->Push(READBARRIER(trueObject));
    } else {
        frame->Push(READBARRIER(falseObject));
    }
}

void _Mutex::New(pVMObject object, pVMFrame frame){
    frame->Pop();
    pVMMutex mutex = _UNIVERSE->NewMutex();
    frame->Push(mutex);
}

_Mutex::_Mutex() :
        PrimitiveContainer() {
    this->SetPrimitive("lock", new
                       Routine<_Mutex>(this, &_Mutex::Lock));
    
    this->SetPrimitive("unlock", new
                       Routine<_Mutex>(this, &_Mutex::Unlock));
    
    this->SetPrimitive("isLocked", new
                       Routine<_Mutex>(this, &_Mutex::IsLocked));
    
    this->SetPrimitive("new", new
                       Routine<_Mutex>(this, &_Mutex::New));
}

