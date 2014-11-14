//
//  VMMutex.h
//  SOM
//
//  Created by Jeroen De Geeter on 5/03/14.
//
//

#ifndef SOM_VMMutex_h
#define SOM_VMMutex_h

#include <pthread.h>
#include <vmobjects/VMObject.h>

typedef pthread_mutex_t MutexId, *pMutexId;

class VMMutex : public VMObject {
    
public:
    typedef GCMutex Stored;
    
    VMMutex();
    
    pMutexId GetEmbeddedMutexId();
    void Lock();
    void Unlock();
    bool IsLocked();
    
private:
    
    MutexId embeddedMutexId;
    
    static const int VMMutexNumberOfFields;
    
};

#endif
