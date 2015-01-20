#pragma once
//
//  VMSignal.h
//  SOM
//
//  Created by Jeroen De Geeter on 5/03/14.
//
//

#include "VMMutex.h"

class VMSignal : public VMObject {
    
public:
    typedef GCSignal Stored;
    
    VMSignal();
    
    pthread_cond_t* GetEmbeddedSignalId();
    pthread_mutex_t* GetEmbeddedMutexId();
    void Wait();
    void Signal();
    bool SignalAll();
    
    virtual VMSignal* Clone(Page*);

#if GC_TYPE==PAUSELESS
    virtual void MarkReferences();
    virtual void CheckMarking(void (vm_oop_t));
#endif
    
private:
    pthread_cond_t  embeddedSignalId;
    pthread_mutex_t embeddedMutexId;
    
    static const int VMSignalNumberOfFields;
    
};
