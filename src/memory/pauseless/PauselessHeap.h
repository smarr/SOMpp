#pragma once

#include "../../misc/defs.h"
#include <assert.h>

#if GC_TYPE == PAUSELESS

#include "../../interpreter/Interpreter.h"
#include <memory/PagedHeap.h>
#include <memory/Page.h>§
#include "../../vmobjects/VMObjectBase.h"
#include "../../vmobjects/AbstractObject.h"
#include "../../vm/Universe.h"

#define MASK_OBJECT_NMT (1 << 1)
#define REFERENCE_NMT_VALUE(REFERENCE) (((reinterpret_cast<uintptr_t>(REFERENCE) & MASK_OBJECT_NMT) == 0) ? false : true)
#define FLIP_NMT_VALUE(REFERENCE) (((reinterpret_cast<intptr_t>(REFERENCE) & MASK_OBJECT_NMT) == 0) ? ((pVMObject)(reinterpret_cast<intptr_t>(REFERENCE) | MASK_OBJECT_NMT)) : ((pVMObject)(reinterpret_cast<intptr_t>(REFERENCE) ^ MASK_OBJECT_NMT)))

class PauselessHeap : public PagedHeap {
 
public:
    PauselessHeap(long objectSpaceSize = HEAP_SIZE, long pageSize = PAGE_SIZE);
    
    void ReadBarrier(void** referenceHolder);
    void NMTTrap(void**);
    void GCTrap(void**);
    
    
    void SignalMarkingOfRootSet(Interpreter*);
    void SignalInterpreterBlocked(Interpreter*);
    
};

inline void PauselessHeap::NMTTrap(void** referenceHolder) {
    if (_UNIVERSE->GetInterpreter()->GetExpectedNMT() != REFERENCE_NMT_VALUE(*referenceHolder)) {
        *referenceHolder = FLIP_NMT_VALUE(*referenceHolder);
        _UNIVERSE->GetInterpreter()->AddGCWork((AbstractVMObject*)*referenceHolder);
    }
}

inline void PauselessHeap::GCTrap(void** referenceHolder) {
    size_t pageNumber = ((size_t)*referenceHolder - (size_t)memoryStart) / pageSize;
    Page* page = allPages->at(pageNumber);
    if (_UNIVERSE->GetInterpreter()->GCTrapEnabled() && page->Blocked())
        // wait till all mutators have enabled their gc-trap
        while (......) {
            pthread_cond_wait()
        }
        pthread_cond_signal();
        *referenceHolder = page->LookupNewAddress((AbstractVMObject*)*referenceHolder);
    }
}

inline void PauselessHeap::ReadBarrier(void** referenceHolder) {
    //assert(*referenceHolder != nullptr);
    if (*referenceHolder == nullptr)
        return;
    GCTrap(referenceHolder);
    NMTTrap(referenceHolder);
}

#endif








//#define REFERENCE_NMT_VALUE(REFERENCE) ((((uintptr_t)REFERENCE & MASK_OBJECT_NMT) == 0) ? false : true)
//#define FLIP_NMT_VALUE(REFERENCE) ((((uintptr_t)REFERENCE & MASK_OBJECT_NMT) == 0) ? ((pVMObject)((uintptr_t)REFERENCE | MASK_OBJECT_NMT)) : ((pVMObject)((uintptr_t)REFERENCE ^ MASK_OBJECT_NMT)))