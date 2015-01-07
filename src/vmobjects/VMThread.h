#pragma once

#include "VMObject.h"

#include <thread>

class VMThread : public VMObject {
public:
    typedef GCThread Stored;
    
    VMThread();

    VMString* GetName() const;
    void      SetName(VMString*);
    
    void Join();
    
    virtual StdString AsDebugString() const;
    virtual VMThread* Clone() const;
    virtual void MarkObjectAsInvalid();
    
    void SetThread(std::thread*);

    static void Yield();
    static VMThread* Current();
    
    static void Initialize();
    static void WalkGlobals(walk_heap_fn walk);
    static void RegisterThread(thread::id, VMThread*);
    static void UnregisterThread(thread::id);

private:
    GCString* name;
    std::thread* thread;
    
    static const int VMThreadNumberOfFields;
    
    static mutex threads_map_mutex;
    static map<thread::id, GCThread*> threads;
};
