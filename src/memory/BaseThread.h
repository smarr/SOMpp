#pragma once

#include <misc/defs.h>

#if GC_TYPE==PAUSELESS
    #include <memory/Worklist.h>
#endif

class BaseThread {

public:
    
    BaseThread(Page*);
#if GC_TYPE==PAUSELESS
    BaseThread(Page*, bool);
#endif
    
    ~BaseThread();
    
    Page* GetPage();
    void SetPage(Page*);
    
#if GC_TYPE==PAUSELESS
    bool GetExpectedNMT();
    virtual void AddGCWork(AbstractVMObject*) = 0;
#endif
    
protected:
    Page* page;
    
#if GC_TYPE==PAUSELESS
    bool expectedNMT;
    Worklist worklist;
#endif
    
};
