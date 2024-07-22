#pragma once

#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMSymbol.h"
#include "SourceCoordinate.h"

class Variable {
public:
    Variable(VMSymbol* name, size_t index, bool isArgument, SourceCoordinate coord) : name(_store_ptr(name)), qualifiedName(_store_ptr(MakeQualifiedName(name, coord))), index(index), isArgument(isArgument), coord(coord) {}
    Variable() : name(nullptr), index(-1) {}
    
    Variable(const Variable* old, size_t newIndex, bool isArgument) : name(old->name), qualifiedName(old->qualifiedName), index(newIndex), isArgument(isArgument), coord(old->coord) {}
    
    void WalkObjects(walk_heap_fn walk);
    
    bool IsValid() const { return name != nullptr; }
    
    inline uint8_t GetIndex() const { return index; }
    
    VMSymbol* GetName() const { return load_ptr(name); }
    VMSymbol* GetQualifiedName() const { return load_ptr(qualifiedName); }
    
    static VMSymbol* MakeQualifiedName(VMSymbol* name, const SourceCoordinate& coord);
    
    
    Variable CopyForInlining(size_t newIndex) const;
    
protected:
    GCSymbol* name;
    GCSymbol* qualifiedName;
    uint8_t index;
    bool isArgument;
    SourceCoordinate coord;
};
