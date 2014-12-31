#pragma once

#include "../../misc/defs.h"
#include <assert.h>

#if GC_TYPE == PAUSELESS

#include "../../interpreter/Interpreter.h"
#include <memory/PagedHeap.h>
#include <memory/Page.h>
#include "../../vmobjects/VMObjectBase.h"
#include "../../vmobjects/AbstractObject.h"
#include "../../vm/Universe.h"
#include "PauselessCollectorThread.h"

#define MASK_OBJECT_NMT (1 << 1)
#define REFERENCE_NMT_VALUE(REFERENCE) (((reinterpret_cast<intptr_t>(REFERENCE) & MASK_OBJECT_NMT) == 0) ? false : true)
#define FLIP_NMT_VALUE(REFERENCE) ( (reinterpret_cast<intptr_t>(REFERENCE) ^ MASK_OBJECT_NMT) )

class PauselessHeap : public PagedHeap {
    friend class PauselessCollectorThread;
 
public:
    PauselessHeap(long, long);
    
    void SignalRootSetMarked();
    void SignalInterpreterBlocked(Interpreter*);
    void SignalSafepointReached(bool*);
    void SignalGCTrapEnabled();
    
    //pthread_mutex_t* GetMarkRootSetMutex();
    //pthread_mutex_t* GetBlockPagesMutex();
    pthread_mutex_t* GetNewInterpreterMutex();
    
    void Start();
    //void Stop();
    
    PauselessCollectorThread* GetGCThread();
    void AddGCThread(PauselessCollectorThread*);
    
    // DIRTY
    inline void* GetMemoryStart() {return memoryStart;}
    inline long GetPageSize() {return pageSize;}
    inline vector<Page*>* GetAllPages() {return allPages;}
    inline int GetNumberOfMutatorsNeedEnableGCTrap() {return numberOfMutatorsNeedEnableGCTrap;}
    inline int GetNumberOfMutatorsWithEnabledGCTrap() {return numberOfMutatorsWithEnabledGCTrap;}
    inline pthread_mutex_t* GetGcTrapEnabledMutex() {return &gcTrapEnabledMutex;}
    inline pthread_cond_t* GetGcTrapEnabledCondition() {return &gcTrapEnabledCondition;}
    inline pthread_mutex_t* GetCompareAndSwapTestMutex() {return &compareAndSwapTestMutex;}
    
    // FOR DEBUGGING PURPOSES
    void Pause();
    void PauseGC();
    bool IsPauseTriggered();
    void TriggerPause();
    void ResetPause();
    int GetCycle();
    int GetMarkValue();
    

private:
    
    //seems a bit illogical to place this here
    pthread_t* threads;
    pthread_key_t pauselessCollectorThread;
    
    pthread_mutex_t gcTrapEnabledMutex;
    pthread_cond_t gcTrapEnabledCondition;
    int numberOfMutatorsNeedEnableGCTrap;
    int numberOfMutatorsWithEnabledGCTrap;
    
    static void* ThreadForGC(void*);
    
    // FOR DEBUGGING PURPOSES
    volatile bool pauseTriggered;
    volatile int readyForPauseThreads;
    pthread_mutex_t doCollect;
    pthread_mutex_t threadCountMutex;
    pthread_cond_t stopTheWorldCondition;
    pthread_cond_t mayProceed;
    pthread_mutex_t compareAndSwapTestMutex;
    
};

template<typename T>
inline T* Flip(T* reference) {
    return (T*) FLIP_NMT_VALUE(reference);
}

template<typename T>
inline typename T::Loaded* Untag(T* reference) {
    if (REFERENCE_NMT_VALUE(reference))
        return (typename T::Loaded*) FLIP_NMT_VALUE(reference);
    else
        return (typename T::Loaded*) reference;
}

template<typename T>
inline typename T::Stored* WriteBarrier(T* reference) {
    if (reference == nullptr)
        return (typename T::Stored*) nullptr;
    if (_UNIVERSE->GetInterpreter()->GetExpectedNMT())
        return (typename T::Stored*) FLIP_NMT_VALUE(reference);
    else
        return (typename T::Stored*) reference;
}

template<typename T>
inline typename T::Stored* WriteBarrierForGCThread(T* reference) {
    if (reference == nullptr)
        return (typename T::Stored*) nullptr;
    if (_HEAP->GetGCThread()->GetExpectedNMT())
        return (typename T::Stored*) FLIP_NMT_VALUE(reference);
    else
        return (typename T::Stored*) reference;
}

template<typename T>
inline typename T::Loaded* ReadBarrier(T** referenceHolder, bool rootSetMarking = false) {
    if (*referenceHolder == nullptr)
        return (typename T::Loaded*)nullptr;
    Interpreter* interpreter = _UNIVERSE->GetInterpreter();
    bool correctNMT = (REFERENCE_NMT_VALUE(*referenceHolder) == interpreter->GetExpectedNMT());
    typename T::Loaded* reference = Untag(*referenceHolder);
    bool trapTriggered = false;
    //gc-trap stuff ------>
    size_t pageNumber = ((size_t)reference - (size_t)(_HEAP->GetMemoryStart())) / _HEAP->GetPageSize();
    Page* page = _HEAP->GetAllPages()->at(pageNumber);
    
    if (interpreter->TriggerGCTrap(page)) {
    //if (interpreter->GCTrapEnabled() && page->Blocked()) {
        pthread_mutex_lock(_HEAP->GetGcTrapEnabledMutex());
        while (_HEAP->GetNumberOfMutatorsNeedEnableGCTrap() != _HEAP->GetNumberOfMutatorsWithEnabledGCTrap()) {
            pthread_cond_wait(_HEAP->GetGcTrapEnabledCondition(), _HEAP->GetGcTrapEnabledMutex());
        }
        pthread_mutex_unlock(_HEAP->GetGcTrapEnabledMutex());
        trapTriggered = true;
        reference = (typename T::Loaded*) page->LookupNewAddress((AbstractVMObject*)reference, interpreter);
    }
    // <-----------
    if (!correctNMT || rootSetMarking)
        interpreter->AddGCWork((AbstractVMObject*)reference);
    if (!correctNMT || trapTriggered) {
        if (interpreter->GetExpectedNMT())
            *referenceHolder = (T*) FLIP_NMT_VALUE(reference);
        else
            *referenceHolder = (T*) reference;
    }
    assert(Universe::IsValidObject((AbstractVMObject*) reference));
    return (typename T::Loaded*) reference;
}

template<typename T>
inline typename T::Loaded* ReadBarrierForGCThread(T** referenceHolder, bool rootSetMarking = false) {
    if (*referenceHolder == nullptr)
        return (typename T::Loaded*)nullptr;
    PauselessCollectorThread* gcThread = _HEAP->GetGCThread();
    bool correctNMT = (REFERENCE_NMT_VALUE(*referenceHolder) == gcThread->GetExpectedNMT());
    typename T::Loaded* reference = Untag(*referenceHolder);
    bool trapTriggered = false;
    //gc-trap stuff ------>
    size_t pageNumber = ((size_t)reference - (size_t)(_HEAP->GetMemoryStart())) / _HEAP->GetPageSize();
    Page* page = _HEAP->GetAllPages()->at(pageNumber);
    if (page->Blocked()) {
        trapTriggered = true;
        reference = (typename T::Loaded*) page->LookupNewAddress((AbstractVMObject*)reference, gcThread);
    }
    // <-----------
    if (!correctNMT || rootSetMarking)
        gcThread->AddGCWork((AbstractVMObject*)reference);
    if (!correctNMT || trapTriggered) {
        if (gcThread->GetExpectedNMT())
            *referenceHolder = (T*) FLIP_NMT_VALUE(reference);
        else
            *referenceHolder = (T*) reference;
    }
    assert(Universe::IsValidObject((AbstractVMObject*) reference));
    return reference;
}

// FOR DEBUGGING PURPOSES
template<typename T>
inline bool GetNMTValue(T* reference) {
    return REFERENCE_NMT_VALUE(reference);
}

template<typename T>
inline void CheckBlocked(T* reference) {
    size_t pageNumber = ((size_t)reference - (size_t)(_HEAP->GetMemoryStart())) / _HEAP->GetPageSize();
    Page* page = _HEAP->allPages->at(pageNumber);
    assert(!page->Blocked());
}

#endif
