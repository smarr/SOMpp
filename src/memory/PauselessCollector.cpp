#include "../misc/defs.h"
#if GC_TYPE==PAUSELESS

#include "PauselessCollector.h"

#include "PagedHeap.h"
#include "../vm/Universe.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMObject.h"
#include "../vmobjects/VMSymbol.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMBlock.h"
#include "../vmobjects/VMPrimitive.h"
#include "../vmobjects/VMClass.h"
#include "../natives/VMThread.h"
#include "../vmobjects/VMEvaluationPrimitive.h"

VMOBJECT_PTR mark_object(VMOBJECT_PTR obj) {
    assert(Universe::IsValidObject(obj));
#ifdef USE_TAGGING
    //don't process tagged objects
    if (IS_TAGGED(obj))
        return obj;
#endif
    if (obj->GetGCField() & MASK_OBJECT_IS_MARKED)
        return (obj);
    
    obj->SetGCField(MASK_OBJECT_IS_OLD | MASK_OBJECT_IS_MARKED);
    obj->WalkObjects(&mark_object);
    return obj;
}

void PauselessCollector::Collect() {
    Timer::GCTimer->Resume();
    
    // Clear current phase mark-bits
    // this could be done by multiple threads
    for (std::vector<Page*>::iterator it = _HEAP->allPages->begin() ; it != _HEAP->allPages->end(); ++it) {
        (*it)->ClearMarkBits();
    }
    
    // Marking of the root-sets
    _UNIVERSE->WalkGlobals(&mark_object);
    vector<Interpreter*>* interpreters = _UNIVERSE->GetInterpreters();
    for (std::vector<Interpreter*>::iterator it = interpreters->begin() ; it != interpreters->end(); ++it) {
        // Get the current frame and thread of each interpreter and mark it.
        // Since marking is done recursively, this automatically
        // marks the whole stack
        pVMFrame currentFrame = (*it)->GetFrame();
        if (currentFrame != NULL) {
            pVMFrame newFrame = static_cast<pVMFrame>(copy_if_necessary(currentFrame));
            (*it)->SetFrame(newFrame);
        }
        pVMThread currentThread = (*it)->GetThread();
        if (currentThread != NULL) {
            pVMThread newThread = static_cast<pVMThread>(copy_if_necessary(currentThread));
            (*it)->SetThread(newThread);
        }
    }
    
    
    
    
    
    
    
    
    
    MinorCollection();
    if (_HEAP->matureObjectsSize > majorCollectionThreshold)
    {
        MajorCollection();
        majorCollectionThreshold = 2 * _HEAP->matureObjectsSize;
        
    }
    
    //reset collection trigger
    heap->resetGCTrigger();
    
    Timer::GCTimer->Halt();
}

void GenerationalCollector::CopyInterpretersFrameAndThread() {
    vector<Interpreter*>* interpreters = _UNIVERSE->GetInterpreters();
    for (std::vector<Interpreter*>::iterator it = interpreters->begin() ; it != interpreters->end(); ++it) {
        // Get the current frame and thread of each interpreter and mark it.
        // Since marking is done recursively, this automatically
        // marks the whole stack
        pVMFrame currentFrame = (*it)->GetFrame();
        if (currentFrame != NULL) {
            pVMFrame newFrame = static_cast<pVMFrame>(copy_if_necessary(currentFrame));
            (*it)->SetFrame(newFrame);
        }
        pVMThread currentThread = (*it)->GetThread();
        if (currentThread != NULL) {
            pVMThread newThread = static_cast<pVMThread>(copy_if_necessary(currentThread));
            (*it)->SetThread(newThread);
        }
    }
}







#endif





/*

VMOBJECT_PTR mark_object(VMOBJECT_PTR obj) {
    assert(Universe::IsValidObject(obj));
#ifdef USE_TAGGING
    //don't process tagged objects
    if (IS_TAGGED(obj))
        return obj;
#endif
    if (obj->GetGCField() & MASK_OBJECT_IS_MARKED)
        return (obj);
    
    obj->SetGCField(MASK_OBJECT_IS_OLD | MASK_OBJECT_IS_MARKED);
    obj->WalkObjects(&mark_object);
    return obj;
}


































VMOBJECT_PTR copy_if_necessary(VMOBJECT_PTR obj) {
    assert(Universe::IsValidObject(obj));
    
#ifdef USE_TAGGING
    //don't process tagged objects
    if (IS_TAGGED(obj))
        return obj;
#endif
    size_t gcField = obj->GetGCField();
    
    // if this is an old object already, we don't have to copy
    if (gcField & MASK_OBJECT_IS_OLD)
        return obj;
    
    // GCField is abused as forwarding pointer here
    // if someone has moved before, return the moved object
    if (gcField != 0)
        return (VMOBJECT_PTR) gcField;
    
    // we have to clone ourselves
    VMOBJECT_PTR newObj = obj->Clone();
    
#ifndef NDEBUG
    obj->MarkObjectAsInvalid();
#endif
    
    assert( (((size_t) newObj) & MASK_OBJECT_IS_MARKED) == 0 );
    assert( obj->GetObjectSize() == newObj->GetObjectSize());
    
    obj->SetGCField((size_t) newObj);
    newObj->SetGCField(MASK_OBJECT_IS_OLD);
    
    // walk recursively
    newObj->WalkObjects(copy_if_necessary);
    return newObj;
}

void GenerationalCollector::MinorCollection() {
    // walk all globals
    _UNIVERSE->WalkGlobals(&copy_if_necessary);
    
    // and the current frames and threads
    CopyInterpretersFrameAndThread();
    
    // and also all objects that have been detected by the write barriers
    for (vector<size_t>::iterator objIter =
         _HEAP->oldObjsWithRefToYoungObjs->begin();
         objIter != _HEAP->oldObjsWithRefToYoungObjs->end();
         objIter++) {
        // content of oldObjsWithRefToYoungObjs is not altered while iteration,
        // because copy_if_necessary returns old objs only -> ignored by
        // write_barrier
        VMOBJECT_PTR obj = (VMOBJECT_PTR)(*objIter);
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
    vector<Interpreter*>* interpreters = _UNIVERSE->GetInterpreters();
    for (std::vector<Interpreter*>::iterator it = interpreters->begin() ; it != interpreters->end(); ++it) {
        Page* newPage = _HEAP->availablePages->back();
        _HEAP->availablePages->pop_back();
        (*it)->SetPage(newPage);
    }
}

void GenerationalCollector::MajorCollection() {
    //first we have to mark all objects (globals and current frame recursively)
    _UNIVERSE->WalkGlobals(&mark_object);
    //and the current frame
    CopyInterpretersFrameAndThread();
    
    //now that all objects are marked we can safely delete all allocated objects that are not marked
    vector<VMOBJECT_PTR>* survivors = new vector<VMOBJECT_PTR>();
    for (vector<VMOBJECT_PTR>::iterator objIter =
         _HEAP->allocatedObjects->begin(); objIter !=
         _HEAP->allocatedObjects->end(); objIter++) {
        
        pVMObject obj = *objIter;
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
    heap->resetGCTrigger();
    
    Timer::GCTimer->Halt();
}

void GenerationalCollector::CopyInterpretersFrameAndThread() {
    vector<Interpreter*>* interpreters = _UNIVERSE->GetInterpreters();
    for (std::vector<Interpreter*>::iterator it = interpreters->begin() ; it != interpreters->end(); ++it) {
        // Get the current frame and thread of each interpreter and mark it.
        // Since marking is done recursively, this automatically
        // marks the whole stack
        pVMFrame currentFrame = (*it)->GetFrame();
        if (currentFrame != NULL) {
            pVMFrame newFrame = static_cast<pVMFrame>(copy_if_necessary(currentFrame));
            (*it)->SetFrame(newFrame);
        }
        pVMThread currentThread = (*it)->GetThread();
        if (currentThread != NULL) {
            pVMThread newThread = static_cast<pVMThread>(copy_if_necessary(currentThread));
            (*it)->SetThread(newThread);
        }
    }
}
*/