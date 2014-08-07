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
    
    void AddWork(VMOBJECT_PTR);
    VMOBJECT_PTR GetWork();
    void MoveWork(Worklist*);
    bool Empty();
    
private:
    pthread_mutex_t lock;
    vector<VMOBJECT_PTR>* work;
    
};

#endif
