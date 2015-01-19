#include <misc/defs.h>
#if GC_TYPE==GENERATIONAL

#include "GenerationalCollector.h"

#include "PagedHeap.h"
#include <interpreter/Interpreter.h>
#include <vm/Universe.h>
#include <vmobjects/VMMethod.h>
#include <vmobjects/VMObject.h>
#include <vmobjects/VMSymbol.h>
#include <vmobjects/VMFrame.h>
#include <vmobjects/VMBlock.h>
#include <vmobjects/VMPrimitive.h>
#include <vmobjects/VMClass.h>
#include <vmobjects/VMThread.h>
#include <vmobjects/VMEvaluationPrimitive.h>

#define INITIAL_MAJOR_COLLECTION_THRESHOLD (5 * 1024 * 1024) //5 MB

GenerationalCollector::GenerationalCollector(PagedHeap* heap) : StopTheWorldCollector(heap) {
    majorCollectionThreshold = INITIAL_MAJOR_COLLECTION_THRESHOLD;
    matureObjectsSize = 0;
}

static gc_oop_t mark_object(gc_oop_t oop) {
    // don't process tagged objects
    if (IS_TAGGED(oop))
        return oop;

    AbstractVMObject* obj = AS_OBJ(oop);
    assert(Universe::IsValidObject(obj));

    if (obj->GetGCField() & MASK_OBJECT_IS_MARKED)
        return oop;

    obj->SetGCField(MASK_OBJECT_IS_OLD | MASK_OBJECT_IS_MARKED);
    obj->WalkObjects(&mark_object);
    return oop;
}

static gc_oop_t copy_if_necessary(gc_oop_t oop) {
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
    AbstractVMObject* newObj = obj->Clone();

    assert( (((uintptr_t) newObj) & MASK_OBJECT_IS_MARKED) == 0 );
    assert( obj->GetObjectSize() == newObj->GetObjectSize());

    if (DEBUG)
        obj->MarkObjectAsInvalid();
    
    obj->SetGCField(reinterpret_cast<intptr_t>(newObj));
    newObj->SetGCField(MASK_OBJECT_IS_OLD);

    // walk recursively
    newObj->WalkObjects(copy_if_necessary);

#warning not sure about the use of _store_ptr here, or whether it should be a plain cast
    return _store_ptr(newObj);
}

void GenerationalCollector::MinorCollection() {
    // walk all globals
    GetUniverse()->WalkGlobals(&copy_if_necessary);

    // and the current frames and threads
    CopyInterpretersFrameAndThread();

    // and also all objects that have been detected by the write barriers
    for (auto obj : *_HEAP->oldObjsWithRefToYoungObjs) {
        // content of oldObjsWithRefToYoungObjs is not altered while iteration,
        // because copy_if_necessary returns old objs only -> ignored by
        // write_barrier
        obj->SetGCField(MASK_OBJECT_IS_OLD);
        obj->WalkObjects(&copy_if_necessary);
    }
    _HEAP->oldObjsWithRefToYoungObjs->clear();
    
    //need to clean this up a bit
    _HEAP->fullPages->erase(_HEAP->fullPages->begin(),_HEAP->fullPages->end());
    for (std::vector<Page*>::iterator it = _HEAP->allPages->begin() ; it != _HEAP->allPages->end(); ++it) {
        (*it)->ClearPage();
        _HEAP->availablePages->push_back((*it));
    }
    vector<Interpreter*>* interpreters = GetUniverse()->GetInterpreters();
    for (std::vector<Interpreter*>::iterator it = interpreters->begin() ; it != interpreters->end(); ++it) {
        Page* newPage = _HEAP->availablePages->back();
        _HEAP->availablePages->pop_back();
        (*it)->SetPage(newPage);
    }
}

void GenerationalCollector::MajorCollection() {
    //first we have to mark all objects (globals and current frame recursively)
    GetUniverse()->WalkGlobals(&mark_object);
    //and the current frame
    CopyInterpretersFrameAndThread();

    //now that all objects are marked we can safely delete all allocated objects that are not marked
    vector<AbstractVMObject*>* survivors = new vector<AbstractVMObject*>();
    for (auto obj : *_HEAP->allocatedObjects) {
        assert(Universe::IsValidObject(obj));
        
        if (obj->GetGCField() & MASK_OBJECT_IS_MARKED) {
            survivors->push_back(obj);
            obj->SetGCField(MASK_OBJECT_IS_OLD);
        }
        else {
            _HEAP->FreeObject(obj);
        }
    }
    delete _HEAP->allocatedObjects;
    _HEAP->allocatedObjects = survivors;
}

void GenerationalCollector::Collect() {
    Timer::GCTimer->Resume();

    MinorCollection();
    if (_HEAP->matureObjectsSize > majorCollectionThreshold)
    {
        MajorCollection();
        majorCollectionThreshold = 2 * _HEAP->matureObjectsSize;

    }
    
    //reset collection trigger
    //heap->resetGCTrigger();
    _HEAP->resetGCTrigger();
    
    Timer::GCTimer->Halt();
}

void GenerationalCollector::CopyInterpretersFrameAndThread() {
    vector<Interpreter*>* interpreters = GetUniverse()->GetInterpreters();
    for (Interpreter* it : *interpreters) {
        // Get the current frame and thread of each interpreter and mark it.
        // Since marking is done recursively, this automatically
        // marks the whole stack
        it->WalkGlobals(copy_if_necessary);
    }
}

#endif
