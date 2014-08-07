#pragma once
#include "../../misc/defs.h"
#if GC_TYPE==PAUSELESS

#include <memory/GarbageCollector.h>
#include "Worklist.h"
#include "../../vm/Universe.h"

#include <pthread.h>

using namespace std;

class PauselessCollector : public GarbageCollector {
    
public:
    PauselessCollector(PagedHeap* heap, int numberOfGCThreads);
    static void MarkObject(VMOBJECT_PTR, Worklist*);
    void RemoveLeftoverInterpreterRootSetBarrier(Interpreter*);
    void RemoveLeftoverInterpreterMarkingBarrier(Interpreter*);
    
private:
    static void* GCThread(void*);
    
    static int numberOfGCThreads;
    static volatile int numberOfGCThreadsDoneMarking;
    
    // do all these need to be volatile?
    static volatile bool doneSignalling;
    static volatile bool doneMarkingGlobals;
    static volatile bool doneBlockingPages;
    
    static pthread_mutex_t blockedMutex;//
    static pthread_mutex_t markGlobalsMutex;//
    static pthread_mutex_t markRootSetsMutex;//
    static pthread_mutex_t leftoverRootSetMutex;//
    static pthread_mutex_t doneMarkingMutex;//
    static pthread_mutex_t blockPagesMutex;//
    
    static pthread_cond_t blockedCondition;//
    static pthread_cond_t doneMarkingRootsCondition;//
    static pthread_cond_t doneMarkingCondition;//
    
    static vector<Interpreter*> interpreters;
    static vector<Interpreter*> blockedInterpreters;
    static vector<Interpreter*> leftoverInterpretersRootSetBarrier;
    static vector<Interpreter*> leftoverInterpretersMarkingBarrier;
    
    vector<Page*> pagesToRelocate;
    
    //volatile int numberOfMarkedRootSets;
};

#endif
