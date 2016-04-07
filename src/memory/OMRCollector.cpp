#include "OMRCollector.h"

#include "../vm/Universe.h"
#include "OMRHeap.h"
#include "../vmobjects/AbstractObject.h"
#include "../vmobjects/VMFrame.h"
#include <vmobjects/IntegerBox.h>
#include "../../omr/include_core/omrvm.h"
#include "../../omr/gc/include/omrgc.h"
#include "../../omr/gc/base/Heap.hpp"

void OMRCollector::Collect() {
    OMRHeap* heap = GetHeap<OMRHeap>();
    Timer::GCTimer->Resume();
    //reset collection trigger
    heap->resetGCTrigger();

    OMR_GC_SystemCollect(heap->thread.omrVMThread, J9MMCONSTANT_EXPLICIT_GC_NOT_AGGRESSIVE);

    MM_Heap *mmheap = MM_GCExtensionsBase::getExtensions(heap->getOMRVM())->heap;
    uintptr_t activeMemorySize = mmheap->getActiveMemorySize();
    uintptr_t survivorsSize = activeMemorySize - mmheap->getActualActiveFreeMemorySize();

    heap->spcAlloc = survivorsSize;
    heap->collectionLimit = activeMemorySize * 0.9;
    Timer::GCTimer->Halt();
}
