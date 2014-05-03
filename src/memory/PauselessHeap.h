#pragma once

#include "../misc/defs.h"
#include <assert.h>
#if GC_TYPE == PAUSELESS

#include "PagedHeap.h"
#include "../vmobjects/VMObjectBase.h"
#include "../vm/Universe.h"

#define MASK_OBJECT_NMT (1 << 1)
#define REFERENCE_NMT_VALUE(REFERENCE) (((REFERENCE & MASK_OBJECT_NMT) == 0) ? false : true)
#define FLIP_NMT_VALUE(REFERENCE) (((REFERENCE & MASK_OBJECT_NMT) == 0) ? (REFERENCE | MASK_OBJECT_NMT) : (REFERENCE ^ MASK_OBJECT_NMT))

class PauselessHeap : public PagedHeap {
 
public:
    PauselessHeap(long objectSpaceSize = 4194304, long pageSize = 32768);
 
    void ReadBarrier(VMOBJECT_PTR reference);
};

inline void PauselessHeap::ReadBarrier(VMOBJECT_PTR* holder) {
    //check if NMT-trap needs to be triggered
    if (_UNIVERSE->GetInterpreter()->GetExpectedNMT() != REFERENCE_NMT_VALUE(*holder)) {
        mark_object(*holder);               //this will still need to be replaced with flagging the gc to do the marking
        holder = FLIP_NMT_VALUE(*holder);   //self healing
    }
    //check if gc-trap needs to be triggered
    if (allPages->at(((size_t)*holder - (size_t)memoryStart) / pageSize)->relocationTriggered) {
        if (reference in side table) {
            
        } else {
            //do relocation self
            
        }
    }
}

#endif
