#include "CopyingHeap.h"
#include "CopyingCollector.h"

#include <interpreter/Interpreter.h>
#include <vm/Universe.h>
#include <vmobjects/AbstractObject.h>
#include <vmobjects/VMObject.h>


CopyingHeap::CopyingHeap(size_t pageSize, size_t maxHeapSize)
    : Heap<CopyingHeap>(new CopyingCollector(this)),
      pageSize(pageSize), maxNumPages(maxHeapSize / pageSize),
      currentNumPages(0) {}

Page* CopyingHeap::RegisterThread() {
    lock_guard<mutex> lock(pages_mutex);
    return reinterpret_cast<Page*>(getNextPage_alreadyLocked());
}

void CopyingHeap::UnregisterThread(Page* page) {
    // NOOP, because we the page is still in use
}

MemoryPage<CopyingHeap>* CopyingHeap::getNextPage_alreadyLocked() {
    MemoryPage<CopyingHeap>* result;

    if (freePages.empty()) {
        currentNumPages++;
        if (currentNumPages > maxNumPages) {
            // during the copy phase, we need twice as many pages, i.e.,
            // a heap twice as large. This is similar to the classic
            // semi-space copy-collector design. Except, that we allow
            // too much allocation.
            if (currentNumPages > maxNumPages * 2) {
                ReachedMaxNumberOfPages(); // won't return
                return nullptr;            // is not executed!
            }
        }
        
        result = new CopyingPage(this);
    } else {
        result = freePages.back();
        freePages.pop_back();
    }
    
    usedPages.push_back(result);
    
    // let's see if we have to trigger the GC
    if (usedPages.size() > 0.9 * maxNumPages) {
        triggerGC();
    }
    
    return result;
}

bool MemoryPage<CopyingHeap>::isFull() {
    return ((uintptr_t) bufferEnd - (uintptr_t) nextFreePosition)
                < 2 * sizeof(VMObject);
}

Page* MemoryPage<CopyingHeap>::GetCurrent() {
    return interpreter->GetPage();
}

#include <vmobjects/VMBlock.h>

void MemoryPage<CopyingHeap>::WalkObjects(walk_heap_fn walk,
                                          Page* target) {
    AbstractVMObject* curObject = static_cast<AbstractVMObject*>(buffer);
    
    while (curObject < nextFreePosition) {
        if (dynamic_cast<VMBlock*>(curObject)) {
            int i = 0;
        }
        curObject->WalkObjects(walk, target);
        curObject = reinterpret_cast<AbstractVMObject*>(
                        (uintptr_t)curObject + curObject->GetObjectSize());
    }
}

void* MemoryPage<CopyingHeap>::allocateInNextPage(size_t size ALLOC_OUTSIDE_NURSERY_DECL) {
    assert(interpreter);
    
    if (next == nullptr) {
        next = heap->getNextPage();
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

void MemoryPage<CopyingHeap>::Reset() {
    next             = nullptr;
    interpreter      = nullptr;
    nextFreePosition = buffer;
    
    if (DEBUG) {
//        memset(buffer, 0, heap->pageSize);
        WalkObjects(invalidate_objects, nullptr);
    }
}

