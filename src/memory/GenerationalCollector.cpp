#include "../misc/defs.h"

#include "GenerationalCollector.h"

#include "Heap.h"
#include "../vm/Universe.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMObject.h"
#include "../vmobjects/VMSymbol.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMBlock.h"
#include "../vmobjects/VMPrimitive.h"
#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMEvaluationPrimitive.h"
#include <vmobjects/IntegerBox.h>

#define INITIAL_MAJOR_COLLECTION_THRESHOLD (5 * 1024 * 1024) //5 MB

GenerationalCollector::GenerationalCollector(GenerationalHeap* heap) : GarbageCollector(heap) {
    majorCollectionThreshold = INITIAL_MAJOR_COLLECTION_THRESHOLD;
    matureObjectsSize = 0;
}

static oop_t mark_object(oop_t oop) {
    // don't process tagged objects
    if (IS_TAGGED(oop))
        return oop;
    
    AbstractVMObject* obj = AS_OBJ(oop);
    assert(Universe::IsValidObject(obj));
    

    if (obj->GetGCField() & MASK_OBJECT_IS_MARKED)
        return (obj);

    obj->SetGCField(MASK_OBJECT_IS_OLD | MASK_OBJECT_IS_MARKED);
    obj->WalkObjects(&mark_object);
    
    return obj;
}

static oop_t copy_if_necessary(oop_t oop) {
    // don't process tagged objects
    if (IS_TAGGED(oop))
        return oop;
    
    AbstractVMObject* obj = AS_OBJ(oop);
    assert(Universe::IsValidObject(obj));


    size_t gcField = obj->GetGCField();

    // if this is an old object already, we don't have to copy
    if (gcField & MASK_OBJECT_IS_OLD)
        return obj;

    // GCField is abused as forwarding pointer here
    // if someone has moved before, return the moved object
    if (gcField != 0)
        return (oop_t) gcField;
    
    // we have to clone ourselves
    AbstractVMObject* newObj = obj->Clone();

    if (DEBUG)
        obj->MarkObjectAsInvalid();

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
    GetUniverse()->WalkGlobals(&copy_if_necessary);

    // and the current frame
    VMFrame* currentFrame = GetUniverse()->GetInterpreter()->GetFrame();
    if (currentFrame != nullptr) {
        VMFrame* newFrame = static_cast<VMFrame*>(copy_if_necessary(currentFrame));
        GetUniverse()->GetInterpreter()->SetFrame(newFrame);
    }

    // and also all objects that have been detected by the write barriers
    for (vector<size_t>::iterator objIter =
            heap->oldObjsWithRefToYoungObjs->begin();
         objIter != heap->oldObjsWithRefToYoungObjs->end();
         objIter++) {
        // content of oldObjsWithRefToYoungObjs is not altered while iteration,
        // because copy_if_necessary returns old objs only -> ignored by
        // write_barrier
        AbstractVMObject* obj = (AbstractVMObject*)(*objIter);
        obj->SetGCField(MASK_OBJECT_IS_OLD);
        obj->WalkObjects(&copy_if_necessary);
    }
    heap->oldObjsWithRefToYoungObjs->clear();
    heap->nextFreePosition = heap->nursery;
}

void GenerationalCollector::MajorCollection() {
    //first we have to mark all objects (globals and current frame recursively)
    GetUniverse()->WalkGlobals(&mark_object);
    //and the current frame
    VMFrame* currentFrame = GetUniverse()->GetInterpreter()->GetFrame();
    if (currentFrame != nullptr) {
        VMFrame* newFrame = static_cast<VMFrame*>(mark_object(currentFrame));
        GetUniverse()->GetInterpreter()->SetFrame(newFrame);
    }

    //now that all objects are marked we can safely delete all allocated objects that are not marked
    vector<AbstractVMObject*>* survivors = new vector<AbstractVMObject*>();
    for (vector<AbstractVMObject*>::iterator objIter =
            heap->allocatedObjects->begin(); objIter !=
            heap->allocatedObjects->end(); objIter++) {
        
        AbstractVMObject* obj = *objIter;
        assert(Universe::IsValidObject(obj));
        
        if (obj->GetGCField() & MASK_OBJECT_IS_MARKED) {
            survivors->push_back(obj);
            obj->SetGCField(MASK_OBJECT_IS_OLD);
        }
        else {
            heap->FreeObject(obj);
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
    if (heap->matureObjectsSize > majorCollectionThreshold)
    {
        MajorCollection();
        majorCollectionThreshold = 2 * heap->matureObjectsSize;

    }
    Timer::GCTimer->Halt();
}
