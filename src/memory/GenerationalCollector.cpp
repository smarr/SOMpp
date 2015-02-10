#include <misc/defs.h>

#include <memory/GenerationalCollector.h>
#include <memory/GenerationalPage.h>

#include <vm/Universe.h>
#include <vmobjects/VMMethod.h>
#include <vmobjects/VMObject.h>
#include <vmobjects/VMSymbol.h>
#include <vmobjects/VMFrame.h>
#include <vmobjects/VMBlock.h>
#include <vmobjects/VMPrimitive.h>
#include <vmobjects/VMClass.h>
#include <vmobjects/VMEvaluationPrimitive.h>
#include <vmobjects/IntegerBox.h>

#define INITIAL_MAJOR_COLLECTION_THRESHOLD (5 * 1024 * 1024) //5 MB

GenerationalCollector::GenerationalCollector(GenerationalHeap* heap)
    : GarbageCollector(heap),
      majorCollectionThreshold(INITIAL_MAJOR_COLLECTION_THRESHOLD) {}

static gc_oop_t mark_object(gc_oop_t oop, Page*) {
    // don't process tagged objects
    if (IS_TAGGED(oop))
        return oop;

    AbstractVMObject* obj = AS_OBJ(oop);
    assert(Universe::IsValidObject(obj));

    if (obj->GetGCField() & MASK_OBJECT_IS_MARKED)
        return oop;

    obj->SetGCField(MASK_OBJECT_IS_OLD | MASK_OBJECT_IS_MARKED);
    obj->WalkObjects(&mark_object, nullptr);

    return oop;
}

static gc_oop_t copy_if_necessary(gc_oop_t oop, Page* page) {
    // don't process tagged objects
    if (IS_TAGGED(oop))
        return oop;
    
    AbstractVMObject* obj = AS_OBJ(oop);

    intptr_t gcField = obj->GetGCField();

    // if this is an old object already, we don't have to copy
    if (gcField & MASK_OBJECT_IS_OLD)
        return oop;

    // GCField is abused as forwarding pointer here
    // if someone has moved before, return the moved object
    if (gcField != 0)
        return (gc_oop_t) gcField;
    
    assert(Universe::IsValidObject(obj));
    
    // we have to clone ourselves
    AbstractVMObject* newObj = obj->Clone(page); // page only used to obtain heap

    assert( (((uintptr_t) newObj) & MASK_OBJECT_IS_MARKED) == 0 );
    
    if (DEBUG && (obj->GetObjectSize() != newObj->GetObjectSize())) {
        cerr << obj->AsDebugString() << endl;
        cerr << newObj->AsDebugString() << endl;
        asm("int3");
        assert(obj->GetObjectSize() == newObj->GetObjectSize());
    }
    assert( obj->GetObjectSize() == newObj->GetObjectSize());

    if (DEBUG)
        obj->MarkObjectAsInvalid();
    
    obj->SetGCField(reinterpret_cast<intptr_t>(newObj));
    newObj->SetGCField(MASK_OBJECT_IS_OLD);

    // walk recursively
    newObj->WalkObjects(copy_if_necessary, page); // page only used to obtain heap

#warning not sure about the use of _store_ptr here, or whether it should be a plain cast
    return _store_ptr(newObj);
}

void GenerationalCollector::MinorCollection() {
    unordered_set<Interpreter*>* interps = GetUniverse()->GetInterpreters();

    // reset pages in interpreters, just to be sure
    Page* page = nullptr;
    for (Interpreter* interp : *interps) {
        page = interp->GetPage();
        interp->SetPage(nullptr);
    }
    
    assert(page);
    
    // walk all globals of universe, and implicily the interpreter
    GetUniverse()->WalkGlobals(&copy_if_necessary, page); // page only used to obtain heap

    // and also all objects that have been detected by the write barriers
    for (NurseryPage* page : heap->usedPages) {
        for (AbstractVMObject* holderObj : page->oldObjsWithRefToYoungObjs) {
            // content of oldObjsWithRefToYoungObjs is not altered while iteration,
            // because copy_if_necessary returns old objs only -> ignored by
            // write_barrier
            assert((holderObj->GetGCField() &
                   (MASK_OBJECT_IS_OLD|MASK_SEEN_BY_WRITE_BARRIER)) == (MASK_OBJECT_IS_OLD|MASK_SEEN_BY_WRITE_BARRIER));
            holderObj->SetGCField(MASK_OBJECT_IS_OLD); // reset the seen by wb bit for next cycle
            
            // obj->SetGCField(MASK_OBJECT_IS_OLD);
            holderObj->WalkObjects(&copy_if_necessary, reinterpret_cast<Page*>(page));
        }
        page->Reset();
        heap->freePages.push_back(page);
    }
    
    heap->usedPages.clear();
    
    // redistribute pages of interpreters, except the last one,
    // which already got a page
    for (auto interp : *interps) {
        auto page = heap->freePages.back();
        assert(page);
        heap->freePages.pop_back();
        interp->SetPage(reinterpret_cast<Page*>(page));
        page->SetInterpreter(interp);
        heap->usedPages.push_back(page);
    }
}

void GenerationalCollector::MajorCollection() {
    // first we have to mark all objects (globals and current frame recursively)
    GetUniverse()->WalkGlobals(&mark_object, nullptr);

    // now that all objects are marked we can safely delete all allocated objects that are not marked
    vector<AbstractVMObject*>* survivors = new vector<AbstractVMObject*>();
    size_t heapSize = 0;
    for (AbstractVMObject* obj : *heap->allocatedObjects) {
        assert(Universe::IsValidObject(obj));
        
        if (obj->GetGCField() & MASK_OBJECT_IS_MARKED) {
            survivors->push_back(obj);
            heapSize += obj->GetObjectSize();
            obj->SetGCField(MASK_OBJECT_IS_OLD);
        }
        else {
            free(obj);
        }
    }
    delete heap->allocatedObjects;
    heap->allocatedObjects = survivors;
}

void GenerationalCollector::Collect() {
    Timer::GCTimer->Resume();
    //reset collection trigger
    heap->resetGCTrigger();

    MinorCollection();
    if (heap->sizeOfMatureObjectHeap > majorCollectionThreshold)
    {
        MajorCollection();
        majorCollectionThreshold = 2 * heap->sizeOfMatureObjectHeap;

    }
    Timer::GCTimer->Halt();
}
