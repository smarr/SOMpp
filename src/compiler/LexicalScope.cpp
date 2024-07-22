#include "../vmobjects/ObjectFormats.h"
#include "LexicalScope.h"


void LexicalScope::WalkObjects(walk_heap_fn walk) {
    for (auto& var : arguments) {
        var.WalkObjects(walk);
    }

    for (auto& var : locals) {
        var.WalkObjects(walk);
    }
}
