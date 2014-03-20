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

typedef pthread_cond_t SignalId, *pSignalId;

class VMSignal : public VMObject {
    
public:
    
    VMSignal();
    
    pSignalId GetEmbeddedSignalId();
    pMutexId  GetEmbeddedMutexId();
    void Wait();
    void Signal();
    bool SignalAll();
    
private:
    SignalId embeddedSignalId;
    MutexId embeddedMutexId;
    
    static const int VMSignalNumberOfFields;
    
};

#endif
