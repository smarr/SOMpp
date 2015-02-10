#pragma once

#include <cstdlib>
#include <cstring>
#include <vector>

#include <memory/MarkSweepHeap.h>


class AbstractVMObject;
class Interpreter;

using namespace std;

class MarkSweepPage {
public:
    MarkSweepPage(MarkSweepHeap* heap) : heap(heap) {
        allocatedObjects = new vector<AbstractVMObject*>();
    }
    
    ~MarkSweepPage() {
        delete allocatedObjects;
    }
    
    void* AllocateObject(size_t size ALLOC_OUTSIDE_NURSERY_DECL) {
        void* newObject = malloc(size);
        
        if (newObject == nullptr) {
            heap->FailedAllocation(size);
        }
        memset(newObject, 0, size);
        
        
        spaceAllocated += size;
        allocatedObjects->push_back(reinterpret_cast<AbstractVMObject*>(newObject));
        
        // let's see if we have to trigger the GC
        if (spaceAllocated >= heap->collectionLimit)
            heap->triggerGC();
        return newObject;
    }
    
    void SetInterpreter(Interpreter*) {};
    
private:
    vector<AbstractVMObject*>* allocatedObjects;
    size_t spaceAllocated;
    MarkSweepHeap* const heap;
    
    friend class MarkSweepHeap;
    friend class MarkSweepCollector;
};
