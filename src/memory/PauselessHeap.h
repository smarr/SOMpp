#pragma once

#include <misc/defs.h>
#include <assert.h>

#include <memory/PagedHeap.h>
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

class PauselessHeap : public PauselessPagedHeap {
    friend class PauselessCollectorThread;
 
public:
    PauselessHeap(size_t pageSize, size_t maxHeapSize);
        
    void SignalRootSetMarked();
    void SignalInterpreterBlocked(Interpreter*);
    void SignalSafepointReached(bool*);
    void SignalGCTrapEnabled();
    
    void Start();
    //void Stop();
    
    PauselessCollectorThread* GetGCThread();
    void AddGCThread(PauselessCollectorThread*);
    
    void AddFullNonRelocatablePage(PauselessPage* page) {
        nonRelocatablePages.push_back(page);
    }
    
    // DIRTY
    FORCE_INLINE void* GetMemoryStart() {return memoryStart;}
    FORCE_INLINE vector<PauselessPage*>* GetAllPages() {return allPages;}
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
    
    Page* RegisterThread() { return reinterpret_cast<Page*>(RequestPage()); /* TODO: do we need to do anything else here? */}
    void UnregisterThread(Page*) { /* TODO: do we need to do anything here? */ }
    

private:
    
    //seems a bit illogical to place this here
    pthread_t* threads;
    pthread_key_t pauselessCollectorThread;
    
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
    if (GetUniverse()->GetInterpreter()->GetExpectedNMT())
        return (typename T::Stored*) FLIP_NMT_VALUE(reference);
    else
        return (typename T::Stored*) reference;
}

template<typename T>
inline typename T::Stored* WriteBarrierForGCThread(T* reference) {
    if (reference == nullptr)
        return (typename T::Stored*) nullptr;
    if (GetHeap<HEAP_CLS>()->GetGCThread()->GetExpectedNMT())
        return (typename T::Stored*) FLIP_NMT_VALUE(reference);
    else
        return (typename T::Stored*) reference;
}

template<typename T>
__attribute__((always_inline)) inline typename T::Loaded* ReadBarrier(T** referenceHolder, bool rootSetMarking = false) {
    typename T::Loaded* reference;
    while (true) {
        T* foo = *referenceHolder;
        if (foo == nullptr)
            return (typename T::Loaded*)nullptr;

        Interpreter* interpreter = GetUniverse()->GetInterpreter();
        bool correctNMT = (REFERENCE_NMT_VALUE(foo) == interpreter->GetExpectedNMT());
        reference = Untag(foo);
        assert(Universe::IsValidObject(reinterpret_cast<vm_oop_t>(reference)));
        
        bool trapTriggered = false;
        //gc-trap stuff ------>
        
        HEAP_CLS* const heap = GetHeap<HEAP_CLS>();
        size_t pageNumber = ((size_t)reference - (size_t)(heap->GetMemoryStart())) / PAGE_SIZE;
        vector<Page*>* allPages = heap->GetAllPages();
        Page* page = (*allPages)[pageNumber];
        
        if (interpreter->TriggerGCTrap(page)) {
            pthread_mutex_lock(heap->GetGcTrapEnabledMutex());
            while (heap->GetNumberOfMutatorsNeedEnableGCTrap() != heap->GetNumberOfMutatorsWithEnabledGCTrap()) {
                pthread_cond_wait(heap->GetGcTrapEnabledCondition(), heap->GetGcTrapEnabledMutex());
            }
            pthread_mutex_unlock(heap->GetGcTrapEnabledMutex());
            trapTriggered = true;
            reference = (typename T::Loaded*) page->LookupNewAddress((AbstractVMObject*)reference, interpreter);
        }
        // <-----------
        if (!correctNMT || trapTriggered) {
            if (interpreter->GetExpectedNMT()) {
                if (! __sync_bool_compare_and_swap(referenceHolder, foo, (T*) FLIP_NMT_VALUE(reference))) {
                    continue;
                }
            } else {
                if (! __sync_bool_compare_and_swap(referenceHolder, foo, (T*) reference)) {
                    continue;
                }
            }
        }
        std::atomic_thread_fence(std::memory_order_seq_cst);
        if (!correctNMT || rootSetMarking)
            interpreter->AddGCWork(reinterpret_cast<AbstractVMObject*>(reference));
        break;
    }
    assert(Universe::IsValidObject(reinterpret_cast<vm_oop_t>(reference)));
    return (typename T::Loaded*) reference;
}

template<typename T>
__attribute__((always_inline)) inline typename T::Loaded* ReadBarrierForGCThread(T** referenceHolder, bool rootSetMarking = false) {
    typename T::Loaded* reference;
    while (true) {
        T* foo = *referenceHolder;
        if (foo == nullptr)
            return (typename T::Loaded*)nullptr;
        PauselessCollectorThread* gcThread = GetHeap<HEAP_CLS>()->GetGCThread();
        bool correctNMT = (REFERENCE_NMT_VALUE(foo) == gcThread->GetExpectedNMT());
        reference = Untag(foo);
        bool trapTriggered = false;
        //gc-trap stuff ------>
        size_t pageNumber = ((size_t)reference - (size_t)(GetHeap<HEAP_CLS>()->GetMemoryStart())) / PAGE_SIZE;
        Page* page = GetHeap<HEAP_CLS>()->GetAllPages()->at(pageNumber);
        if (page->Blocked()) {
            trapTriggered = true;
            reference = (typename T::Loaded*) page->LookupNewAddress((AbstractVMObject*)reference, gcThread);
        }
        // <-----------
        if (!correctNMT || trapTriggered) {
            if (gcThread->GetExpectedNMT()) {
                if (! __sync_bool_compare_and_swap(referenceHolder, foo, (T*) FLIP_NMT_VALUE(reference))) {
                    continue;
                }
            } else {
                if (! __sync_bool_compare_and_swap(referenceHolder, foo, (T*) reference)) {
                    continue;
                }
            }
        }
        std::atomic_thread_fence(std::memory_order_seq_cst);
        
        if (!correctNMT || rootSetMarking)
            gcThread->AddGCWork((AbstractVMObject*)reference);
        break;
    }
    assert(Universe::IsValidObject(reinterpret_cast<vm_oop_t>(reference)));
    return reference;
}

// FOR DEBUGGING PURPOSES
template<typename T>
inline bool GetNMTValue(T* reference) {
    return REFERENCE_NMT_VALUE(reference);
}

template<typename T>
inline void CheckBlocked(T* reference) {
    size_t pageNumber = ((size_t)reference - (size_t)(GetHeap<HEAP_CLS>()->GetMemoryStart())) / PAGE_SIZE;
    Page* page = GetHeap<HEAP_CLS>()->allPages->at(pageNumber);
    assert(!page->Blocked());
}

#endif
