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
 
public:
    PauselessHeap(long, long);
    
    void SignalRootSetMarked();
    void SignalInterpreterBlocked(Interpreter*);
    void SignalSafepointReached();
    void SignalGCTrapEnabled();
    
    void Start();
    
    PauselessCollectorThread* GetGCThread();
    void AddGCThread(PauselessCollectorThread*);
    
private:
    
    pthread_key_t pauselessCollectorThread;
    pthread_mutex_t gcTrapEnabledMutex;
    pthread_cond_t gcTrapEnabledCondition;
    
    static void* ThreadForGC(void*);
    
};

template<typename T>
inline T* FLIP(T* reference) {
    return (T*) FLIP_NMT_VALUE(reference);
}

template<typename T>
inline T* WriteBarrier(T* reference) {
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
        interpreter->AddGCWork((AbstractVMObject*)*referenceHolder);
        *referenceHolder = (T*)FLIP_NMT_VALUE(*referenceHolder); //COMPARE AND SWAP NEEDED
    }
}

template<typename T>
inline void NMTTrapForGCThread(T** referenceHolder) {
    PauselessCollectorThread* gcThread = _HEAP->GetGCThread();
    if (gcThread->GetExpectedNMT() != REFERENCE_NMT_VALUE(*referenceHolder)) {
        gcThread->AddGCWork((AbstractVMObject*)*referenceHolder);
        *referenceHolder = (T*)FLIP_NMT_VALUE(*referenceHolder); //COMPARE AND SWAP NEEDED
    }
}

template<typename T>
inline T* ReadBarrier(T** referenceHolder) {
    if (*referenceHolder == nullptr)
        return (T*)nullptr;
    NMTTrap(referenceHolder);
    return Untag(*referenceHolder);
}

template<typename T>
inline T* ReadBarrierForGCThread(T** referenceHolder) {
    if (*referenceHolder == nullptr)
        return (T*)nullptr;
    //GCTrapGC(referenceHolder);
    NMTTrapForGCThread(referenceHolder);
    return Untag(*referenceHolder);
}

/*
inline void NMTTrap(void** referenceHolder) {
    Interpreter* interpreter = _UNIVERSE->GetInterpreter();
    if (interpreter->GetExpectedNMT() != REFERENCE_NMT_VALUE(*referenceHolder)) {
        cout << "NMT trap activated" << endl;
        interpreter->AddGCWork((AbstractVMObject*)*referenceHolder);
        *referenceHolder = FLIP_NMT_VALUE(*referenceHolder); //COMPARE AND SWAP NEEDED
    }
}

inline AbstractVMObject* Untag(void* reference) {
    if (REFERENCE_NMT_VALUE(reference))
        return (AbstractVMObject*)FLIP_NMT_VALUE(reference);
    else
        return (AbstractVMObject*)reference;
}

inline AbstractVMObject* ReadBarrier(void** referenceHolder) {
    if (*referenceHolder == nullptr)
        return (AbstractVMObject*)nullptr;
    //GCTrap(referenceHolder);
    NMTTrap(referenceHolder);
    return Untag(*referenceHolder);
} */


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
