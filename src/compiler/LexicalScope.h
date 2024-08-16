#pragma once

#include <utility>

#include "../vmobjects/ObjectFormats.h"
#include "Variable.h"

class LexicalScope {
    friend class MethodGenerationContext;

public:
    LexicalScope(LexicalScope* outer, vector<Variable> arguments,
                 vector<Variable> locals)
        : outer(outer), arguments(std::move(arguments)),
          locals(std::move(locals)) {}

    inline size_t GetNumberOfArguments() const { return arguments.size(); }

    inline size_t GetNumberOfLocals() const { return locals.size(); }

    void AddInlinedLocal(Variable& var) {
        assert(var.GetIndex() == locals.size());
        locals.push_back(var);
    }

    const Variable* GetArgument(size_t index, size_t contextLevel) {
        if (contextLevel > 0) {
            return outer->GetArgument(index, contextLevel - 1);
        }

        return &arguments.at(index);
    }

    const Variable* GetLocal(size_t index, uint8_t ctxLevel) {
        if (ctxLevel > 0) {
            return outer->GetLocal(index, ctxLevel - 1);
        }
        return &locals.at(index);
    }

    /**
     * This removes the inlined scope from the chain.
     * Removal is done exactly once, after all embedded blocks
     * were adapted.
     */
    void DropInlinedScope() {
        assert(outer != nullptr);
        assert(outer->outer != nullptr);

        LexicalScope* newOuter = outer->outer;
        outer = newOuter;
    }

private:
    LexicalScope* outer;
    vector<Variable> arguments;
    vector<Variable> locals;
};
