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

void _Mutex::Lock(VMObject* object, VMFrame* frame){
    VMMutex* mutex = (VMMutex*)frame->Pop();
    mutex->Lock();
    frame->Push(mutex);
}

void _Mutex::Unlock(VMObject* object, VMFrame* frame){
    VMMutex* mutex = (VMMutex*)frame->Pop();
    mutex->Unlock();
    frame->Push(mutex);
}

void _Mutex::IsLocked(VMObject* object, VMFrame* frame){
    VMMutex* mutex = (VMMutex*)frame->Pop();
    if (mutex->IsLocked()) {
        frame->Push(READBARRIER(trueObject));
    } else {
        frame->Push(READBARRIER(falseObject));
    }
}

void _Mutex::New(VMObject* object, VMFrame* frame){
    frame->Pop();
    VMMutex* mutex = GetUniverse()->NewMutex();
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

