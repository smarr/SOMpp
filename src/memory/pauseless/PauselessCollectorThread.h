#pragma once
#include "../../misc/defs.h"
#if GC_TYPE==PAUSELESS

#include <memory/GarbageCollector.h>
#include "../BaseThread.h"
#include "Worklist.h"
#include "../../vm/Universe.h"

#include <pthread.h>

using namespace std;

class PauselessCollectorThread: public BaseThread {
    
public:
    
    PauselessCollectorThread();
    
    static void SetNumberOfGCThreads(int);
    
    static void MarkObject(VMOBJECT_PTR);
    static void AddBlockedInterpreter(Interpreter*);
    static void SignalRootSetMarked();
    static void AddNonEmptyWorklist(Worklist*);
    static void SignalSafepointReached();
    
    void Collect();
    
    virtual void AddGCWork(AbstractVMObject*);
    
private:
    
    static int numberOfGCThreads;
    
    // variables used during root-set marking phase
    static pthread_mutex_t blockedMutex;
    static pthread_mutex_t markGlobalsMutex;
    static pthread_mutex_t markRootSetsMutex;
    static pthread_mutex_t markRootSetsCheckpointMutex;
    static pthread_cond_t  doneMarkingRootSetsCondition;
    static pthread_cond_t  blockedCondition;
    static int numberRootSetsMarked;
    static int numberRootSetsToBeMarked;
    static bool doneSignalling;
    static bool doneMarkingGlobals;
    static vector<Interpreter*>* blockedInterpreters;
    
    // variables used during the rest of the marking phase
    static pthread_mutex_t nonEmptyWorklistsMutex;
    static pthread_cond_t  waitingForWorkCondition;
    static vector<Worklist*>* nonEmptyWorklists;
    static bool doneRequestCheckpoint;
    static int numberOfGCThreadsDoneMarking;
    static atomic<int> numberOfMutatorsPassedSafepoint;
    static int numberOfMutators;
    
    // variables used during the relocate phase
    static bool doneBlockingPages;
    static pthread_mutex_t blockPagesMutex;
    static pthread_mutex_t pagesToRelocateMutex;
    static pthread_cond_t pagesToRelocateCondition;
    static vector<Page*>* pagesToRelocate;
    static vector<Page*>* pagesToUnblock;
    
    
    // TEST VARIABLES
    static pthread_mutex_t endMutex;
    static pthread_cond_t endCondition;
    static int numberReachingEnd;
    static bool done;
    static int numberOfCycles;
};

#endif
