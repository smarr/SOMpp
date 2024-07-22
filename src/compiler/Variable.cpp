#include <cassert>
#include <cstddef>
#include <cstdio>
#include <string>

#include "../vm/Symbols.h"
#include "SourceCoordinate.h"
#include "Variable.h"

std::string Variable::MakeQualifiedName() const {
    char qualified[100];
    assert(name.size() < 80);

    snprintf(qualified, 100, "%.*s:%zu:%zu",
             (int)name.size(), name.data(),
             coord.GetLine(), coord.GetColumn());

    return {qualified};
}

Variable Variable::CopyForInlining(size_t newIndex) const {
    if (isArgument) {
        if (name == strBlockSelf) {
            return Variable();
        }
        return Variable(this, newIndex, true);
    }
    return Variable(this, newIndex, false);
}
