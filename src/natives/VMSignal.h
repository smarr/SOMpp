//
//  VMSignal.h
//  SOM
//
//  Created by Jeroen De Geeter on 5/03/14.
//
//

#ifndef SOM_VMSignal_h
#define SOM_VMSignal_h

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
    virtual pVMSignal Clone(Interpreter*);
    virtual pVMSignal Clone(PauselessCollectorThread*);
    virtual void MarkReferences();
    virtual void CheckMarking(void (AbstractVMObject*));
#else
    virtual pVMSignal Clone();
    virtual void WalkObjects(VMOBJECT_PTR (VMOBJECT_PTR));
#endif
    
private:
    pthread_cond_t  embeddedSignalId;
    pthread_mutex_t embeddedMutexId;
    
    static const int VMSignalNumberOfFields;
    
};

#endif
