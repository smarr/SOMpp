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
#endif
    
private:
    
    pthread_mutex_t embeddedMutexId;
    
    static const int VMMutexNumberOfFields;
    
};

#endif
