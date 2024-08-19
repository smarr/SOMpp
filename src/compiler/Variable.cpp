#include "Variable.h"

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <string>

#include "../vm/Print.h"
#include "../vm/Symbols.h"
#include "SourceCoordinate.h"

std::string Variable::MakeQualifiedName() const {
    char qualified[100];
    assert(name.size() < 80);

    if (snprintf(qualified, 100, "%.*s:%zu:%zu", (int)name.size(), name.data(),
                 coord.GetLine(), coord.GetColumn()) < 0) {
        ErrorExit("failed to MakeQualifiedName");
    }

    return {qualified};
}

Variable Variable::CopyForInlining(size_t newIndex) const {
    if (isArgument) {
        if (name == strBlockSelf) {
            // that's invalid
            return {};
        }
    }
    // arguments that are inlined need to turn into variables, too
    return {this, newIndex, false};
}
