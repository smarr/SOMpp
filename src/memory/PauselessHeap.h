#pragma once

#include <misc/defs.h>
#include <assert.h>

#include <memory/PauselessPage.h>
#include <vmobjects/VMObjectBase.h>
#include <vmobjects/AbstractObject.h>
#include <vm/Universe.h>
#include "PauselessCollectorThread.h"

#include <interpreter/Interpreter.h>

class PauselessCollectorThread;

#define MASK_OBJECT_NMT (1 << 1)
#define REFERENCE_NMT_VALUE(REFERENCE) (((reinterpret_cast<intptr_t>(REFERENCE) & MASK_OBJECT_NMT) == 0) ? false : true)
#define FLIP_NMT_VALUE(REFERENCE) ( (reinterpret_cast<intptr_t>(REFERENCE) ^ MASK_OBJECT_NMT) )

class PauselessHeap : public Heap<PauselessHeap> {
    friend class PauselessCollectorThread;
 
public:
    PauselessHeap(size_t pageSize, size_t maxHeapSize);
        
    void SignalRootSetMarked();
    void SignalInterpreterBlocked(Interpreter*);
    void SignalSafepointReached(bool*);
    void SignalGCTrapEnabled();
    
    void Start();
    //void Stop();
        
    void AddFullNonRelocatablePage(PauselessPage* page) {
        nonRelocatablePages.push_back(page);
    }
    
    bool HasReachedMaxPageCount(size_t currentNumPages, size_t maxNumPages) {
        return currentNumPages > maxNumPages;
    }
    
    inline int GetNumberOfMutatorsNeedEnableGCTrap() {return numberOfMutatorsNeedEnableGCTrap;}
    inline int GetNumberOfMutatorsWithEnabledGCTrap() {return numberOfMutatorsWithEnabledGCTrap;}
    inline pthread_mutex_t* GetGcTrapEnabledMutex() {return &gcTrapEnabledMutex;}
    inline pthread_cond_t* GetGcTrapEnabledCondition() {return &gcTrapEnabledCondition;}
    
    // FOR DEBUGGING PURPOSES
    void Pause();
    void PauseGC();
    bool IsPauseTriggered();
    void TriggerPause();
    void ResetPause();
    int GetCycle();
    int GetMarkValue();
    
    Page* RegisterThread() { return reinterpret_cast<Page*>(pagedHeap.GetNextPage()); /* TODO: do we need to do anything else here? */}
    void UnregisterThread(Page*) { /* TODO: do we need to do anything here? */ }
    
    inline PauselessPage* GetPageFromObj(AbstractVMObject* obj) {
        uintptr_t bits = reinterpret_cast<uintptr_t>(obj);
        uintptr_t mask = ~(pagedHeap.pageSize - 1);
        uintptr_t page = bits & mask;
        return reinterpret_cast<PauselessPage*>(page);
    }

    PagedHeap<PauselessPage, PauselessHeap> pagedHeap;

private:
    
    //seems a bit illogical to place this here
    pthread_t* threads;
    
    pthread_mutex_t gcTrapEnabledMutex;
    pthread_cond_t gcTrapEnabledCondition;
    int numberOfMutatorsNeedEnableGCTrap;
    int numberOfMutatorsWithEnabledGCTrap;
    
    vector<PauselessPage*> nonRelocatablePages;
    
    static void* ThreadForGC(void*);
    
    // FOR DEBUGGING PURPOSES
    volatile bool pauseTriggered;
    volatile int readyForPauseThreads;
    pthread_mutex_t doCollect;
    pthread_mutex_t threadCountMutex;
    pthread_cond_t stopTheWorldCondition;
    pthread_cond_t mayProceed;
    
    friend PauselessPage;
};

template<typename T>
inline T* Flip(T* reference) {
    return (T*) FLIP_NMT_VALUE(reference);
}

template<typename T>
FORCE_INLINE typename T::Loaded* Untag(T* reference) {
    if (REFERENCE_NMT_VALUE(reference))
        return (typename T::Loaded*) FLIP_NMT_VALUE(reference);
    else
        return (typename T::Loaded*) reference;
}


#if GC_TYPE == PAUSELESS
template<typename T>
inline typename T::Stored* toGcOop(T* reference) {
    if (reference == nullptr)
        return (typename T::Stored*) nullptr;
    if (GetUniverse()->GetBaseThread()->GetExpectedNMT())
        return (typename T::Stored*) FLIP_NMT_VALUE(reference);
    else
        return (typename T::Stored*) reference;
}

template<typename T>
__attribute__((always_inline)) inline typename T::Loaded* ReadBarrier(T** referenceHolder, bool rootSetMarking = false) {
    AbstractVMObject* reference;
    while (true) {
        if (IS_TAGGED(*referenceHolder))
            return reinterpret_cast<typename T::Loaded*>(*referenceHolder);

        GCAbstractObject* foo = reinterpret_cast<GCAbstractObject*>(*referenceHolder);
        if (foo == nullptr)
            return nullptr;

        BaseThread* thread = GetUniverse()->GetBaseThread();
        bool correctNMT = (REFERENCE_NMT_VALUE(foo) == thread->GetExpectedNMT());
        reference = Untag(foo);
        assert(Universe::IsValidObject(reinterpret_cast<vm_oop_t>(reference)));
        
        bool trapTriggered = false;
        //gc-trap stuff ------>
        
        PauselessHeap* const heap = GetHeap<PauselessHeap>();
        PauselessPage* page = heap->GetPageFromObj(reference);

        // make sure the GC thread never triggers GC trap, should alway return false
        assert(dynamic_cast<PauselessCollectorThread*>(thread) == nullptr || thread->TriggerGCTrap(page) == false);
        
        if (thread->TriggerGCTrap(page)) {
            pthread_mutex_lock(heap->GetGcTrapEnabledMutex());
            while (heap->GetNumberOfMutatorsNeedEnableGCTrap() != heap->GetNumberOfMutatorsWithEnabledGCTrap()) {
                pthread_cond_wait(heap->GetGcTrapEnabledCondition(), heap->GetGcTrapEnabledMutex());
            }
            pthread_mutex_unlock(heap->GetGcTrapEnabledMutex());
            trapTriggered = true;
            reference = page->LookupNewAddress(reference, thread);
        }
        // <-----------
        if (!correctNMT || trapTriggered) {
            if (thread->GetExpectedNMT()) {
                if (! __sync_bool_compare_and_swap(referenceHolder,
                                                   reinterpret_cast<T*>(foo),
                                                   reinterpret_cast<T*>(FLIP_NMT_VALUE(reference)))) {
                    continue;
                }
            } else {
                if (! __sync_bool_compare_and_swap(referenceHolder,
                                                   reinterpret_cast<T*>(foo),
                                                   reinterpret_cast<T*>(reference))) {
                    continue;
                }
            }
        }
        std::atomic_thread_fence(std::memory_order_seq_cst);
        if (!correctNMT || rootSetMarking)
            thread->AddGCWork(reinterpret_cast<AbstractVMObject*>(reference));
        break;
    }
    assert(Universe::IsValidObject(reinterpret_cast<vm_oop_t>(reference)));
    return (typename T::Loaded*) reference;
}

// FOR DEBUGGING PURPOSES
template<typename T>
inline bool GetNMTValue(T* reference) {
    return REFERENCE_NMT_VALUE(reference);
}

inline void CheckBlocked(vm_oop_t reference) {
    if (IS_TAGGED(reference))
        return;
    
    PauselessHeap* const heap = GetHeap<PauselessHeap>();
    PauselessPage* page = heap->GetPageFromObj(
                            reinterpret_cast<AbstractVMObject*>(reference));
    assert(!page->Blocked());
}

#endif
