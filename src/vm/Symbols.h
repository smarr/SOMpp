#pragma once

#include <string>

#include "../vmobjects/VMSymbol.h"

VMSymbol* SymbolFor(const std::string& str);

void InitializeSymbols();

void WalkSymbols(walk_heap_fn walk);

#ifdef UNITTESTS
VMSymbol* NewSymbol(const std::string& str);
VMSymbol* NewSymbol(const size_t length, const char* str);
#endif

extern GCSymbol* symbolIfTrue;
extern GCSymbol* symbolIfFalse;
