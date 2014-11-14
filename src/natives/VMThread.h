//
//  VMThread.h
//  SOM
//
//  Created by Jeroen De Geeter on 5/03/14.
//
//

#ifndef SOM_VMThread_h
#define SOM_VMThread_h

#include "VMSignal.h"
#include "../vmObjects/ObjectFormats.h"

typedef pthread_t ThreadId;
typedef pthread_key_t ThreadSafeGlobal;

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
    ThreadId    GetEmbeddedThreadId();
    void        SetEmbeddedThreadId(ThreadId value);
    
    int GetThreadId();
    void SetThreadId(int);
    
    void        Join(int* exitStatus);
    
private:
    
    GCSignal* resumeSignal;
    GCObject* shouldStop;
    GCBlock*  blockToRun;
    GCString* name;
    GCAbstractObject* argument;
    ThreadId embeddedThreadId;
    //int threadId;
    
    static const int VMThreadNumberOfFields;
    
};

#endif
