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
    void        Join(int* exitStatus);
    
private:
    
    pVMSignal resumeSignal;
    pVMObject shouldStop;
    pVMBlock  blockToRun;
    pVMString name;
    pVMObject argument;
    ThreadId embeddedThreadId;
    
    static const int VMThreadNumberOfFields;
    
};

#endif
