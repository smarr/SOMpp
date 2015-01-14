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
    
    static void InitializeCollector(int);
    
    static void MarkObject(VMOBJECT_PTR);
    static void AddBlockedInterpreter(Interpreter*);
    static void SignalRootSetMarked();
    static void AddNonEmptyWorklist(Worklist*);
    static void SignalSafepointReached(bool*);
    
    static pthread_mutex_t* GetNewInterpreterMutex();
    
    void Collect();
    
    virtual void AddGCWork(AbstractVMObject*);
    
    static int GetCycle();
    static int GetMarkValue();
    
private:
    
    static int numberOfGCThreads;
    
    static int markValue;
    
    // variables used so that new mutator threads can be created
    static pthread_mutex_t newInterpreterMutex;
    
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
    static vector<Worklist*>* nonEmptyWorklists;
    static pthread_mutex_t nonEmptyWorklistsMutex;
    static pthread_mutexattr_t attrNonEmptyWorklistsMutex;
    static pthread_cond_t  waitingForWorkCondition;
    static pthread_cond_t  waitingForCheckpointCondition;
    static int numberOfGCThreadsDoneMarking;
    static int numberOfMutatorsPassedSafepoint;
    static int numberOfMutators;
    static bool checkpointRequested;
    static bool checkpointFinished;
    
    // variables used during the relocate phase
    static bool doneBlockingPages;
    static pthread_mutex_t blockPagesMutex;
    static pthread_mutex_t pagesToRelocateMutex;
    static pthread_cond_t pagesToRelocateCondition;
    static vector<Page*>* pagesToRelocate;
    static vector<Page*>* pagesToUnblock;
    
    // end of cycle variables
    static int numberOfGCThreadsFinished;
    static pthread_mutex_t endOfCycleMutex;
    static pthread_cond_t endOfCycleCond;
    static bool endBool;
    
    // COLLECTECTING STATISTICS
    static int numberOfCycles;
    
    // FOR DEBUGGING PURPOSES
    static void CheckMarkingOfObject(VMOBJECT_PTR);
    static void CheckMarking();
};

//static pthread_mutex_t signalRootSetMarkingMutex;
//static pthread_mutex_t SignalGCTrapMutex;

//static pthread_mutex_t* GetMarkRootSetMutex();
//static pthread_mutex_t* GetBlockPagesMutex();

#endif
