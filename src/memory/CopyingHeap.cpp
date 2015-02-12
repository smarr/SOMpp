#include "CopyingHeap.h"
#include "CopyingCollector.h"

#include <interpreter/Interpreter.h>
#include <vm/Universe.h>
#include <vmobjects/AbstractObject.h>
#include <vmobjects/VMObject.h>
#include <vmobjects/IntegerBox.h>
#include <vmobjects/VMInteger.h>


CopyingHeap::CopyingHeap(size_t pageSize, size_t maxHeapSize)
    : Heap<CopyingHeap>(new CopyingCollector(this)),
      pagedHeap(this, pageSize, maxHeapSize / pageSize) {}

Page* CopyingHeap::RegisterThread() {
    return reinterpret_cast<Page*>(pagedHeap.GetNextPage());
}

void CopyingHeap::UnregisterThread(Page* page) {
    // NOOP, because we the page is still in use
}

Page* CopyingPage::GetCurrent() {
    return interpreter->GetPage();
}

#include <vmobjects/VMBlock.h>

void CopyingPage::WalkObjects(walk_heap_fn walk, Page* target) {
    AbstractVMObject* curObject = static_cast<AbstractVMObject*>(buffer);
    
    while (curObject < nextFreePosition) {
        curObject->WalkObjects(walk, target);
        curObject = reinterpret_cast<AbstractVMObject*>(
                        (uintptr_t)curObject + curObject->GetObjectSize());
    }
}

void* CopyingPage::allocateInNextPage(size_t size ALLOC_OUTSIDE_NURSERY_DECLpp) {
    assert(interpreter);
    
    if (next == nullptr) {
        next = heap->pagedHeap.GetNextPage();
        next->SetInterpreter(interpreter);
        
        // need to set the page unconditionally, even if it is not yet
        // completely full. Otherwise, we can have the situation that during
        // the copying phase, an object gets moved to a previous page, and
        // is missed while moving all dependent objects, and thus, not moving
        // its dependent objects.
        interpreter->SetPage(reinterpret_cast<Page*>(next));
    }
    
    return next->AllocateObject(size ALLOC_HINT);
}

static gc_oop_t invalidate_objects(gc_oop_t oop, Page*) {
    if (IS_TAGGED(oop)) {
        return oop;
    }
    
    AbstractVMObject* obj = AS_OBJ(oop);
    obj->MarkObjectAsInvalid();
    return oop;
}

void CopyingPage::Reset() {
    next             = nullptr;
    interpreter      = nullptr;
    nextFreePosition = buffer;
    
    if (DEBUG) {
//        memset(buffer, 0, heap->pageSize);
        WalkObjects(invalidate_objects, nullptr);
    }
}

