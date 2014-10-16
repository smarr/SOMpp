#pragma once

#if GC_TYPE == PAUSELESS

#include <vector>
#include <pthread.h>

#include "../../vmobjects/ObjectFormats.h"

class AbstractVMObject;

using namespace std;

class Worklist {
    
public:
    Worklist();
    ~Worklist();
    
    void AddWork(AbstractVMObject*);
    AbstractVMObject* GetWork();
    void MoveWork(Worklist*);
    bool Empty();
    
private:
    pthread_mutex_t lock;
    vector<AbstractVMObject*> work;
    
};

#endif
