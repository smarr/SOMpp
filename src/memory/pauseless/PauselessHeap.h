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
    void SignalSafepointReached();
    void SignalGCTrapEnabled();
    
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
    
    // FOR DEBUGGING PURPOSES
    void Pause();
    bool IsPauseTriggered();
    void TriggerPause();
    void ResetPause();

private:
    
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
    
};

template<typename T>
inline T* Flip(T* reference) {
    return (T*) FLIP_NMT_VALUE(reference);
}

template<typename T>
inline T* WriteBarrier(T* reference) {
    if (reference == nullptr)
        return (T*)nullptr;
    if (_UNIVERSE->GetInterpreter()->GetExpectedNMT())
        return (T*) FLIP_NMT_VALUE(reference);
    else
        return reference;
}

template<typename T>
inline T* WriteBarrierForGCThread(T* reference) {
    if (_HEAP->GetGCThread()->GetExpectedNMT())
        return (T*) FLIP_NMT_VALUE(reference);
    else
        return reference;
}

template<typename T>
inline T* Untag(T* reference) {
    if (REFERENCE_NMT_VALUE(reference))
        return (T*)FLIP_NMT_VALUE(reference);
    else
        return reference;
}

template<typename T>
inline void NMTTrap(T** referenceHolder) {
    Interpreter* interpreter = _UNIVERSE->GetInterpreter();
    if (interpreter->GetExpectedNMT() != REFERENCE_NMT_VALUE(*referenceHolder)) {
        interpreter->AddGCWork((AbstractVMObject*)Untag(*referenceHolder));
        *referenceHolder = (T*)FLIP_NMT_VALUE(*referenceHolder);
    }
}

template<typename T>
inline void NMTTrapForGCThread(T** referenceHolder) {
    PauselessCollectorThread* gcThread = _HEAP->GetGCThread();
    if (gcThread->GetExpectedNMT() != REFERENCE_NMT_VALUE(*referenceHolder)) {
        gcThread->AddGCWork((AbstractVMObject*)Untag(*referenceHolder));
        *referenceHolder = (T*)FLIP_NMT_VALUE(*referenceHolder);
    }
}

template<typename T>
inline void GCTrap(T** referenceHolder) {
    assert(Universe::IsValidObject((AbstractVMObject*) Untag(*referenceHolder)));
    
    size_t pageNumber = ((size_t)Untag(*referenceHolder) - (size_t)(_HEAP->GetMemoryStart())) / _HEAP->GetPageSize();
    Page* page = _HEAP->GetAllPages()->at(pageNumber);
    if (_UNIVERSE->GetInterpreter()->GCTrapEnabled() && page->Blocked()) {
        pthread_mutex_lock(_HEAP->GetGcTrapEnabledMutex());
        while (_HEAP->GetNumberOfMutatorsNeedEnableGCTrap() != _HEAP->GetNumberOfMutatorsWithEnabledGCTrap()) {
            pthread_cond_wait(_HEAP->GetGcTrapEnabledCondition(), _HEAP->GetGcTrapEnabledMutex());
        }
        pthread_mutex_unlock(_HEAP->GetGcTrapEnabledMutex());
        *referenceHolder = (T*)page->LookupNewAddress((AbstractVMObject*)Untag(*referenceHolder), _UNIVERSE->GetInterpreter());
    }
}

template<typename T>
inline void GCTrapForGCThread(T** referenceHolder) {
    assert(Universe::IsValidObject((AbstractVMObject*) Untag(*referenceHolder)));
    
    size_t pageNumber = ((size_t)Untag(*referenceHolder) - (size_t)(_HEAP->GetMemoryStart())) / _HEAP->GetPageSize();
    Page* page = _HEAP->GetAllPages()->at(pageNumber);
    if (page->Blocked()) {
        /*while (_HEAP->GetNumberOfMutatorsNeedEnableGCTrap() != _HEAP->GetNumberOfMutatorsWithEnabledGCTrap()) {
            pthread_cond_wait(_HEAP->GetGcTrapEnabledCondition(), _HEAP->GetGcTrapEnabledMutex());
        } */
        *referenceHolder = (T*)page->LookupNewAddress((AbstractVMObject*)Untag(*referenceHolder), _HEAP->GetGCThread());
    }
}

template<typename T>
inline T* ReadBarrier(T** referenceHolder) {
    if (*referenceHolder == nullptr)
        return (T*)nullptr;
    assert(Universe::IsValidObject((AbstractVMObject*) Untag(*referenceHolder)));
    size_t pageNumber = ((size_t)Untag(*referenceHolder) - (size_t)(_HEAP->GetMemoryStart())) / _HEAP->GetPageSize();
    assert(pageNumber < 7680);
    GCTrap(referenceHolder);
    NMTTrap(referenceHolder);
    
    T* res = Untag(*referenceHolder);
    assert(Universe::IsValidObject((AbstractVMObject*) res));
    return res;
}

template<typename T>
inline T* ReadBarrierForGCThread(T** referenceHolder) {
    if (*referenceHolder == nullptr)
        return (T*)nullptr;
    assert(Universe::IsValidObject((AbstractVMObject*) Untag(*referenceHolder)));
    
    size_t pageNumber = ((size_t)Untag(*referenceHolder) - (size_t)(_HEAP->GetMemoryStart())) / _HEAP->GetPageSize();
    assert(pageNumber < 7680);
    GCTrapForGCThread(referenceHolder);
    NMTTrapForGCThread(referenceHolder);
    
    T* res = Untag(*referenceHolder);
    assert(Universe::IsValidObject((AbstractVMObject*) res));
    return res;
}





/*
inline void GCTrap(void** referenceHolder) {
    size_t pageNumber = ((size_t)*referenceHolder - (size_t)memoryStart) / pageSize;
    Page* page = allPages->at(pageNumber);
    if (_UNIVERSE->GetInterpreter()->GCTrapEnabled() && page->Blocked()) {
        //and all mutators have there gc trap enabled
        //pthread_mutex_lock(&gcTrapEnabledMutex);
        //while (!) {
        //    pthread_cond_wait(&gcTrapEnabledCondition, &gcTrapEnabledMutex);
        //}
        //pthread_mutex_unlock(&gcTrapEnabledMutex);
        *referenceHolder = page->LookupNewAddress((AbstractVMObject*)*referenceHolder);
    }
} */


/*
inline void GCTrapGC(void** referenceHolder) {
    size_t pageNumber = ((size_t)*referenceHolder - (size_t)memoryStart) / pageSize;
    Page* page = allPages->at(pageNumber);
    if (page->Blocked()) {
        *referenceHolder = page->LookupNewAddress((AbstractVMObject*)*referenceHolder);
    }
} */




#endif
