#include <misc/defs.h>

#include "CopyingHeap.h"
#include <vm/Universe.h>
#include <vmobjects/AbstractObject.h>
#include <vmobjects/VMFrame.h>
#include <vmobjects/IntegerBox.h>

#include "CopyingCollector.h"

static gc_oop_t copy_if_necessary(gc_oop_t oop, Page* target) {
    // don't process tagged objects
    if (IS_TAGGED(oop))
        return oop;
    
    assert(oop != nullptr);
    
    AbstractVMObject* obj = AS_OBJ(oop);

    intptr_t gcField = obj->GetGCField();
    // GCField is abused as forwarding pointer here
    // if someone has moved before, return the moved object
    if (gcField != 0)
        return (gc_oop_t) gcField;
    
    assert(Universe::IsValidObject(obj));
    
    // we have to clone ourselves
    AbstractVMObject* newObj = obj->Clone(target);
    
    if (DEBUG)
        obj->MarkObjectAsInvalid();
    
    obj->SetGCField(reinterpret_cast<intptr_t>(newObj));
    assert(newObj->GetGCField() == 0);
    
#warning not sure about the use of _store_ptr here, or whether it should be a plain cast
    return _store_ptr(newObj);
}

static gc_oop_t objects_are_valid(gc_oop_t oop, Page*) {
    // don't process tagged objects
    if (IS_TAGGED(oop))
        return oop;

    AbstractVMObject* obj = AS_OBJ(oop);
    assert(Universe::IsValidObject(obj));
    
    return oop;
}

void CopyingCollector::Collect() {
    Timer::GCTimer->Resume();

    // reset collection trigger
    heap->resetGCTrigger();
    
    vector<CopyingPage*> oldPages(heap->usedPages); // TODO: can we use move constructor here?
    heap->usedPages.clear();
    
    unordered_set<Interpreter*>* interps = GetUniverse()->GetInterpreters();
    
    // reset pages in interpreters, just to be sure
    // use last interpreter as the one for the GC
    Interpreter* lastI = nullptr;
    for (Interpreter* interp : *interps) {
        interp->SetPage(nullptr);
        lastI = interp;
    }
    
    
    CopyingPage* target = heap->getNextPage_alreadyLocked();
    target->SetInterpreter(lastI);
    lastI->SetPage(reinterpret_cast<Page*>(target));
    GetUniverse()->WalkGlobals(copy_if_necessary, reinterpret_cast<Page*>(target));

    
    // now copy all objects that are referenced by the objects were moved
    // note: we use iteration with a custom loop and [] because the vector
    //       usedPages can be modified while copying object to include the
    //       new pages of copied objects, which we also still need to travers
    size_t i = 0;
    while (i < heap->usedPages.size()) {
        heap->usedPages[i]->WalkObjects(
                            copy_if_necessary, target->GetCurrent());
        i++;
    }
    
    for (auto page : oldPages) {
        page->Reset();
        heap->freePages.push_back(page);
    }
    
    if (DEBUG) {
        GetUniverse()->WalkGlobals(objects_are_valid, nullptr);
        
        size_t j = 0;
        // let's test whether all objects are valid
        for (auto page : heap->usedPages) {
            page->WalkObjects(objects_are_valid, nullptr);
            j++;
        }
        assert(i == j);
    }
    
    // redistribute pages of interpreters, except the last one,
    // which already got a page
    for (auto interp : *interps) {
        if (interp != lastI) {
            auto page = heap->freePages.back();
            assert(page);
            heap->freePages.pop_back();
            interp->SetPage(reinterpret_cast<Page*>(page));
            page->SetInterpreter(interp);
            heap->usedPages.push_back(page);
        }
    }
    

    Timer::GCTimer->Halt();
}
