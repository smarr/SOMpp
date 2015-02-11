#include "Worklist.h"
#include "PauselessCollectorThread.h"
#include <assert.h>

#if GC_TYPE == PAUSELESS


void Worklist::AddWorkGC(AbstractVMObject* reference) {
    lock_guard<mutex> lock(mtx);
    work.push_back(reference);
}

void Worklist::AddWorkMutator(AbstractVMObject* reference) {
    bool wasEmpty;
    {
        lock_guard<mutex> lock(mtx);
        wasEmpty = work.empty();
        work.push_back(reference);
    }
    if (wasEmpty) {
        PauselessCollectorThread::AddNonEmptyWorklist(this);
    }
}

AbstractVMObject* Worklist::GetWork() {
    lock_guard<mutex> lock(mtx);
    
    AbstractVMObject* reference = work.back();
    work.pop_back();
    
    return reference;
}

void Worklist::MoveWork(Worklist* moveToWorklist) {
    lock_guard<mutex> lock(mtx);
    
    while (!work.empty()) {
        moveToWorklist->AddWorkGC(work.back());
        work.pop_back();
    }
}

#endif
