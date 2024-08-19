#pragma once

#include <string>

#include "../vmobjects/VMSymbol.h"

VMSymbol* SymbolFor(const std::string& str);

void InitializeSymbols();

void WalkSymbols(walk_heap_fn walk);

#ifdef UNITTESTS
VMSymbol* NewSymbol(const std::string& str);
VMSymbol* NewSymbol(size_t length, const char* str);
#endif

extern GCSymbol* symbolSelf;
extern GCSymbol* symbolSuper;
extern GCSymbol* symbolBlockSelf;

extern GCSymbol* symbolIfTrue;
extern GCSymbol* symbolIfFalse;

extern GCSymbol* symbolPlus;
extern GCSymbol* symbolMinus;

const char* const strBlockSelf = "$blockSelf";
const char* const strSuper = "super";
const char* const strSelf = "self";
