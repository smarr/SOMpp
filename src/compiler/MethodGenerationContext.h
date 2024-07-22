#pragma once

/*
 *
 *
 Copyright (c) 2007 Michael Haupt, Tobias Pape, Arne Bergmann
 Software Architecture Group, Hasso Plattner Institute, Potsdam, Germany
 http://www.hpi.uni-potsdam.de/swa/

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

#include <vector>

#include "../misc/defs.h"
#include "../vmobjects/ObjectFormats.h"
#include "ClassGenerationContext.h"
#include "LexicalScope.h"
#include "SourceCoordinate.h"

class Parser;

class MethodGenerationContext {
public:
    MethodGenerationContext(ClassGenerationContext& holder, MethodGenerationContext* outer = nullptr);
    ~MethodGenerationContext();

    VMMethod* Assemble();
    VMPrimitive* AssemblePrimitive(bool classSide);

    int8_t FindLiteralIndex(vm_oop_t lit);
    bool FindVar(VMSymbol* var, int64_t* index,
            int* context, bool* isArgument);
    bool HasField(VMSymbol* field);

    uint8_t GetFieldIndex(VMSymbol* field);

    void SetSignature(VMSymbol* sig);
    void AddArgument(VMSymbol* arg, const SourceCoordinate& coord);
    void SetPrimitive(bool prim = true);
    void AddLocal(VMSymbol* local, const SourceCoordinate& coord);
    uint8_t AddLiteral(vm_oop_t lit);
    void UpdateLiteral(vm_oop_t oldValue, uint8_t index, vm_oop_t newValue);
    bool AddArgumentIfAbsent(VMSymbol* arg, const SourceCoordinate& coord);
    bool AddLocalIfAbsent(VMSymbol* local, const SourceCoordinate& coord);
    int8_t AddLiteralIfAbsent(vm_oop_t lit);
    void SetFinished(bool finished = true);

    ClassGenerationContext* GetHolder() const {
        return &holderGenc;
    }

    MethodGenerationContext* GetOuter() const {
        return outerGenc;
    }

    uint8_t GetMaxContextLevel() const { return maxContextLevel; }

    VMSymbol* GetSignature() {
        return signature;
    }

    bool IsPrimitive() const {
        return primitive;
    }

    bool IsBlockMethod() const {
        return blockMethod;
    }

    bool IsFinished() const {
        return finished;
    }

    void RemoveLastBytecode() {bytecode.pop_back();};
    size_t GetNumberOfArguments();
    void AddBytecode(uint8_t bc, size_t stackEffect);
    void AddBytecodeArgument(uint8_t bc);

    bool HasBytecodes();

    std::vector<uint8_t> GetBytecodes() {
        return bytecode;
    }

    void CompleteLexicalScope();

private:
    ClassGenerationContext& holderGenc;
    MethodGenerationContext* const outerGenc;

    const uint8_t maxContextLevel;

    const bool blockMethod;
    VMSymbol* signature;
    std::vector<Variable> arguments;
    bool primitive;
    std::vector<Variable> locals;
    std::vector<vm_oop_t> literals;
    bool finished;
    std::vector<uint8_t> bytecode;
    LexicalScope* lexicalScope;

    size_t currentStackDepth;
    size_t maxStackDepth;
};
