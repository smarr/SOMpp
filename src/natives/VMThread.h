#pragma once
//
//  VMThread.h
//  SOM
//
//  Created by Jeroen De Geeter on 5/03/14.
//
//

#include "VMSignal.h"
#include "../vmObjects/ObjectFormats.h"

class VMThread : public VMObject {
    
public:
    typedef GCThread Stored;
    
    VMThread();
    
    static void     Yield();
    
    pVMSignal   GetResumeSignal();
    void        SetResumeSignal(pVMSignal value);
    bool        ShouldStop();
    void        SetShouldStop(bool value);
    pVMBlock    GetBlockToRun();
    void        SetBlockToRun(pVMBlock value);
    pVMString   GetName();
    void        SetName(pVMString value);
    pVMObject   GetArgument();
    void        SetArgument(pVMObject value);
    pthread_t   GetEmbeddedThreadId();
    void        SetEmbeddedThreadId(pthread_t value);
    
    int GetThreadId();
    void SetThreadId(int);
    
    void        Join(int* exitStatus);
    
#if GC_TYPE==PAUSELESS
    virtual pVMThread Clone(Interpreter*);
    virtual pVMThread Clone(PauselessCollectorThread*);
    virtual void MarkReferences();
    virtual void CheckMarking(void (AbstractVMObject*));
#else
    //virtual pVMThread Clone();
    //virtual void WalkObjects(VMOBJECT_PTR (VMOBJECT_PTR));
#endif
    
private:
    
    GCSignal* resumeSignal;
    GCObject* shouldStop;
    GCBlock*  blockToRun;
    GCString* name;
    GCAbstractObject* argument;
    pthread_t embeddedThreadId;
    
    static const int VMThreadNumberOfFields;
    
};
