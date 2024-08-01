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

#include <array>
#include <vector>

#include "../misc/defs.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMMethod.h"
#include "ClassGenerationContext.h"
#include "LexicalScope.h"
#include "SourceCoordinate.h"

class Parser;

class MethodGenerationContext {
public:
    MethodGenerationContext(ClassGenerationContext& holder,
                            MethodGenerationContext* outer = nullptr);
    ~MethodGenerationContext();

    VMMethod* Assemble();
    VMPrimitive* AssemblePrimitive(bool classSide);

    int8_t FindLiteralIndex(vm_oop_t lit);
    bool FindVar(std::string& var, int64_t* index, int* context,
                 bool* isArgument);
    bool HasField(VMSymbol* field);

    uint8_t GetFieldIndex(VMSymbol* field);

    void SetSignature(VMSymbol* sig);

    void AddArgument(std::string& arg, const SourceCoordinate& coord);
    void AddLocal(std::string& local, const SourceCoordinate& coord);
    bool AddArgumentIfAbsent(std::string& arg, const SourceCoordinate& coord);
    bool AddLocalIfAbsent(std::string& local, const SourceCoordinate& coord);

    void SetPrimitive(bool prim = true);

    uint8_t AddLiteral(vm_oop_t lit);
    int8_t AddLiteralIfAbsent(vm_oop_t lit);
    void UpdateLiteral(vm_oop_t oldValue, uint8_t index, vm_oop_t newValue);

    void SetFinished(bool finished = true);

    ClassGenerationContext* GetHolder() const { return &holderGenc; }

    MethodGenerationContext* GetOuter() const { return outerGenc; }

    uint8_t GetMaxContextLevel() const { return maxContextLevel; }

    VMSymbol* GetSignature() { return signature; }

    bool IsPrimitive() const { return primitive; }

    bool IsBlockMethod() const { return blockMethod; }

    bool IsFinished() const { return finished; }

    void RemoveLastBytecode() { bytecode.pop_back(); };
    size_t GetNumberOfArguments();
    void AddBytecode(uint8_t bc, size_t stackEffect);
    void AddBytecodeArgument(uint8_t bc);
    size_t AddBytecodeArgumentAndGetIndex(uint8_t bc);

    bool HasBytecodes();

    std::vector<uint8_t> GetBytecodes() { return bytecode; }

    bool InlineWhile(Parser& parser, bool isWhileTrue);
    bool InlineIfTrueOrIfFalse(bool isIfTrue);

    inline size_t OffsetOfNextInstruction() { return bytecode.size(); }

    void CompleteLexicalScope();
    void MergeIntoScope(LexicalScope& scopeToBeInlined);

    uint8_t GetInlinedLocalIdx(const Variable* var) const;

    void EmitBackwardsJumpOffsetToTarget(size_t loopBeginIdx);
    void PatchJumpOffsetToPointToNextInstruction(size_t indexOfOffset);

private:
    void removeLastBytecodes(size_t numBytecodes);
    bool hasOneLiteralBlockArgument();
    bool hasTwoLiteralBlockArguments();
    bool lastBytecodeIs(size_t indexFromEnd, uint8_t bytecode);
    std::tuple<vm_oop_t, vm_oop_t> extractBlockMethodsAndRemoveBytecodes();
    vm_oop_t extractBlockMethodAndRemoveBytecode();

    vm_oop_t getLastBlockMethodAndFreeLiteral(uint8_t blockLiteralIdx);

    void completeJumpsAndEmitReturningNil(Parser& parser, size_t loopBeginIdx,
                                          size_t jumpOffsetIdxToSkipLoopBody);
    void inlineLocals(LexicalScope& scopeToBeInlined);
    void checkJumpOffset(size_t jumpOffset, uint8_t bytecode);
    void resetLastBytecodeBuffer();

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

    std::array<uint8_t, 4> last4Bytecodes;

    std::vector<BackJump> inlinedLoops;

    bool isCurrentlyInliningABlock;
};
