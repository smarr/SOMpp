#pragma once

//
//  VMMutex.h
//  SOM
//
//  Created by Jeroen De Geeter on 5/03/14.
//
//

#include <pthread.h>
#include <vmobjects/VMObject.h>

class VMMutex : public VMObject {
    
public:
    typedef GCMutex Stored;
    
    VMMutex();
    
    pthread_mutex_t* GetEmbeddedMutexId();
    void Lock();
    void Unlock();
    bool IsLocked();
    
#if GC_TYPE==PAUSELESS
    virtual VMMutex* Clone(Interpreter*);
    virtual VMMutex* Clone(PauselessCollectorThread*);
    virtual void MarkReferences();
    virtual void CheckMarking(void (vm_oop_t));
#else
    virtual VMMutex* Clone();
    virtual void WalkObjects(walk_heap_fn walk);
#endif
    
private:
    
    pthread_mutex_t embeddedMutexId;
    
    static const int VMMutexNumberOfFields;
    
};
