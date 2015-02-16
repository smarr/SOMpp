#pragma once

#include <misc/defs.h>
#include <misc/SpinLock.h>

#if GC_TYPE==PAUSELESS
    #include <memory/Worklist.h>
#endif

class BaseThread {
public:

#if GC_TYPE==PAUSELESS
    BaseThread(Page*, bool);
    BaseThread(Page*, bool, bool);
#else
    BaseThread(Page*);
#endif
    
    ~BaseThread();
    
    Page* GetPage();
    void SetPage(Page*);
    
#if GC_TYPE==PAUSELESS
    bool GetExpectedNMT();
    virtual void AddGCWork(AbstractVMObject*) = 0;
    
    bool GCTrapEnabled();
    bool TriggerGCTrap(Page*);
#endif
    
protected:
    Page* page;
    
#if GC_TYPE==PAUSELESS
    bool expectedNMT;
    Worklist worklist;

    bool gcTrapEnabled;
    SpinLock gcTrap_mutex;
#endif

};
