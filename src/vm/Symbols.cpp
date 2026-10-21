#include "Symbols.h"

#include <cstddef>
#include <map>
#include <string>

#include "../memory/Heap.h"
#include "../misc/defs.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMSymbol.h"
#include "Statistics.h"

static map<std::string, GCSymbol*> symbolsMap;

GCSymbol* symbolSelf;
GCSymbol* symbolSuper;
GCSymbol* symbolBlockSelf;

GCSymbol* symbolIfTrue;
GCSymbol* symbolIfFalse;

GCSymbol* symbolPlus;
GCSymbol* symbolMinus;

VMSymbol* NewSymbol(const size_t length, const char* str) {
    auto* result =
        new (GetHeap<HEAP_CLS>(), PADDED_SIZE(length)) VMSymbol(length, str);
    symbolsMap[std::string(str, length)] = store_root(result);

    recordStat(Allocation, "VMSymbol", result->GetObjectSize());
    return result;
}

VMSymbol* NewSymbol(const std::string& str) {
    return NewSymbol(str.length(), str.c_str());
}

VMSymbol* SymbolFor(const std::string& str) {
    auto const it = symbolsMap.find(str);
    return (it == symbolsMap.end()) ? NewSymbol(str) : load_ptr(it->second);
}

void InitializeSymbols() {
    symbolSelf = store_root(SymbolFor("self"));
    symbolSuper = store_root(SymbolFor("super"));
    symbolBlockSelf = store_root(SymbolFor("$blockSelf"));
    symbolIfTrue = store_root(SymbolFor("ifTrue:"));
    symbolIfFalse = store_root(SymbolFor("ifFalse:"));

    symbolPlus = store_root(SymbolFor("+"));
    symbolMinus = store_root(SymbolFor("-"));
}

void WalkSymbols(walk_heap_fn walk) {
    // walk all entries in symbols map
    map<std::string, GCSymbol*>::iterator symbolIter;
    for (symbolIter = symbolsMap.begin(); symbolIter != symbolsMap.end();
         symbolIter++) {
        // insert overwrites old entries inside the internal map
        walk_ptr(symbolIter->second, walk);
    }

    // reassign symbols
    walk_ptr(symbolSelf, walk);
    walk_ptr(symbolSuper, walk);
    walk_ptr(symbolBlockSelf, walk);
    walk_ptr(symbolIfTrue, walk);
    walk_ptr(symbolIfFalse, walk);

    walk_ptr(symbolPlus, walk);
    walk_ptr(symbolMinus, walk);
}
