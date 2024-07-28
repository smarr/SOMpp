#include <cstddef>
#include <map>
#include <string>

#include "../memory/Heap.h"
#include "../misc/defs.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMSymbol.h"
#include "LogAllocation.h"
#include "Symbols.h"

map<std::string, GCSymbol*> symbolsMap;

GCSymbol* symbolIfTrue;
GCSymbol* symbolIfFalse;

VMSymbol* NewSymbol(const size_t length, const char* str) {
    VMSymbol* result = new (GetHeap<HEAP_CLS>(), PADDED_SIZE(length)) VMSymbol(length, str);
    symbolsMap[StdString(str, length)] = store_root(result);

    LOG_ALLOCATION("VMSymbol", result->GetObjectSize());
    return result;
}

VMSymbol* NewSymbol(const std::string& str) {
    return NewSymbol(str.length(), str.c_str());
}

VMSymbol* SymbolFor(const std::string& str) {
    map<string,GCSymbol*>::iterator it = symbolsMap.find(str);
    return (it == symbolsMap.end()) ? NewSymbol(str) : load_ptr(it->second);
}

void InitializeSymbols() {
    symbolIfTrue  = store_root(SymbolFor("ifTrue:"));
    symbolIfFalse = store_root(SymbolFor("ifFalse:"));
}

void WalkSymbols(walk_heap_fn walk) {
    // walk all entries in symbols map
    map<StdString, GCSymbol*>::iterator symbolIter;
    for (symbolIter = symbolsMap.begin();
         symbolIter != symbolsMap.end();
         symbolIter++) {
        //insert overwrites old entries inside the internal map
        symbolIter->second = static_cast<GCSymbol*>(walk(symbolIter->second));
    }

    //reassign ifTrue ifFalse Symbols
    symbolIfTrue  = symbolsMap["ifTrue:"];
    symbolIfFalse = symbolsMap["ifFalse:"];
}
