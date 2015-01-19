#pragma once
//
//  VMThread.h
//  SOM
//
//  Created by Jeroen De Geeter on 5/03/14.
//
//

#include "VMSignal.h"
#include <vmObjects/ObjectFormats.h>

class VMThread : public VMObject {
public:
    typedef GCThread Stored;
    
    VMThread();
    
    static void     Yield();
    
    VMSignal*   GetResumeSignal();
    void        SetResumeSignal(VMSignal* value);
    bool        ShouldStop();
    void        SetShouldStop(bool value);
    VMBlock*    GetBlockToRun();
    void        SetBlockToRun(VMBlock* value);
    VMString*   GetName();
    void        SetName(VMString* value);
    vm_oop_t    GetArgument();
    void        SetArgument(vm_oop_t value);
    pthread_t   GetEmbeddedThreadId();
    void        SetEmbeddedThreadId(pthread_t value);
    
    int GetThreadId();
    void SetThreadId(int);
    
    void        Join(int* exitStatus);
    
#if GC_TYPE==PAUSELESS
    virtual VMThread* Clone(Interpreter*);
    virtual VMThread* Clone(PauselessCollectorThread*);
    virtual void MarkReferences();
    virtual void CheckMarking(void (vm_oop_t));
#else
    //virtual VMThread* Clone();
    //virtual void WalkObjects(walk_heap_fn walk);
#endif
    
private:
    
    GCSignal* resumeSignal;
    GCObject* shouldStop;
    GCBlock*  blockToRun;
    GCString* name;
    gc_oop_t  argument;
    pthread_t embeddedThreadId;
    
    static const int VMThreadNumberOfFields;
    
};
