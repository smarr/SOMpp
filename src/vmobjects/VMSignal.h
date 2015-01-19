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
    
#if GC_TYPE==PAUSELESS
    virtual VMSignal* Clone(Interpreter*);
    virtual VMSignal* Clone(PauselessCollectorThread*);
    virtual void MarkReferences();
    virtual void CheckMarking(void (vm_oop_t));
#else
    virtual VMSignal* Clone();
#endif
    
private:
    pthread_cond_t  embeddedSignalId;
    pthread_mutex_t embeddedMutexId;
    
    static const int VMSignalNumberOfFields;
    
};
