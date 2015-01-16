#pragma once
#include "../../misc/defs.h"
#if GC_TYPE==PAUSELESS

#include <memory/GarbageCollector.h>
#include "Worklist.h"
#include "../../vm/Universe.h"

#include <pthread.h>

using namespace std;

class PauselessCollector : public GarbageCollector {
    //friend PagedHeap;
    
public:
    PauselessCollector(PagedHeap* heap, int numberOfGCThreads);
    static void MarkObject(vm_oop_t, Worklist*);
    void AddBlockedInterpreter(Interpreter*);
    void SignalRootSetMarked();
    void AddNonEmptyWorklist(Worklist*);
    void SignalSafepointReached();
    
    void Start();
    
private:
    
    static void RelocatePage();
    
    static void* GCThread(void*);
    
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
    
};

#endif
