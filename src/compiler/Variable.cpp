#include <cassert>
#include <cstddef>
#include <cstdio>

#include "../vm/Symbols.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMSymbol.h"
#include "SourceCoordinate.h"
#include "Variable.h"

VMSymbol* Variable::MakeQualifiedName(VMSymbol* name, const SourceCoordinate& coord) {
    char qualified[100];
    assert(name->GetStringLength() < 80);

    snprintf(qualified, 100, "%.*s:%zu:%zu",
             (int)name->GetStringLength(), name->GetRawChars(),
             coord.GetLine(), coord.GetColumn());

    return SymbolFor(qualified);
}

void Variable::WalkObjects(walk_heap_fn walk) {
    name = static_cast<GCSymbol*>(walk(name));
    qualifiedName = static_cast<GCSymbol*>(walk(name));
}

Variable Variable::CopyForInlining(size_t newIndex) const {
    if (isArgument) {
        if (load_ptr(name) == load_ptr(symbolBlockSelf)) {
            return Variable();
        }
        return Variable(this, newIndex, true);
    }
    return Variable(this, newIndex, false);
}
