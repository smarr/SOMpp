#pragma once

#include "../misc/defs.h"
#include <assert.h>

#include "Heap.h"

#include "vm/Universe.h"

#if GC_TYPE == OMR_GARBAGE_COLLECTION
#include "../../omr/include_core/omr.h"
#include "../../omrglue/LanguageThreadLocalHeapStruct.h"

class OMRHeap : public Heap<OMRHeap> {
    friend class OMRCollector;
public:
    OMRHeap(long objectSpaceSize = 1048576);
    AbstractVMObject* AllocateObject(size_t size);
    void writeBarrier(AbstractVMObject* holder, vm_oop_t referencedObject);
    long getHeapSize() {return heapSize;}
    SOM_VM *getVM() {return &vm;}
    SOM_Thread *getThread() {return &thread;}
    OMR_VM *getOMRVM() {return vm.omrVM;}
protected:

private:
    size_t spcAlloc;
    long collectionLimit;
    long heapSize;
    SOM_VM vm;
    SOM_Thread thread;
};

inline void OMRHeap::writeBarrier(AbstractVMObject* holder, vm_oop_t referencedObject) {
	assert(Universe::IsValidObject(referencedObject));
	assert(Universe::IsValidObject((vm_oop_t) holder));
}

#endif
