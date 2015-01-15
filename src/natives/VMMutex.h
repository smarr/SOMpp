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
    virtual pVMMutex Clone(Interpreter*);
    virtual pVMMutex Clone(PauselessCollectorThread*);
    virtual void MarkReferences();
    virtual void CheckMarking(void (AbstractVMObject*));
#else
    virtual pVMMutex Clone();
    virtual void WalkObjects(VMOBJECT_PTR (VMOBJECT_PTR));
#endif
    
private:
    
    pthread_mutex_t embeddedMutexId;
    
    static const int VMMutexNumberOfFields;
    
};
