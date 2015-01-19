#pragma once

#if GC_TYPE == PAUSELESS

#include <vector>
#include <mutex>

#include <vmobjects/ObjectFormats.h>

class AbstractVMObject;

using namespace std;

class Worklist {
    
public:    
    void AddWorkGC(AbstractVMObject*);
    void AddWorkMutator(AbstractVMObject*);
    AbstractVMObject* GetWork();
    void MoveWork(Worklist*);
    
    inline bool Empty() { return work.empty(); }

    void Clear() {
        lock_guard<mutex> lock(mtx);
        work.clear();
    }
    
private:
    mutex mtx;
    vector<AbstractVMObject*> work;
    
};

#endif
