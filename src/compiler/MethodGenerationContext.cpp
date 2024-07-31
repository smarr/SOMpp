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

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <tuple>
#include <vector>

#include "../interpreter/bytecodes.h"
#include "../misc/VectorUtil.h"
#include "../vm/Globals.h"
#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMPrimitive.h"
#include "../vmobjects/VMSymbol.h"
#include "BytecodeGenerator.h"
#include "ClassGenerationContext.h"
#include "LexicalScope.h"
#include "MethodGenerationContext.h"
#include "SourceCoordinate.h"
#include "Variable.h"

MethodGenerationContext::MethodGenerationContext(ClassGenerationContext& holder, MethodGenerationContext* outer) :
        holderGenc(holder), outerGenc(outer),
        blockMethod(outer != nullptr), signature(nullptr), primitive(false), finished(false),
        currentStackDepth(0), maxStackDepth(0), maxContextLevel(outer == nullptr ? 0 : outer->GetMaxContextLevel() + 1) {
            last4Bytecodes = {BC_INVALID, BC_INVALID, BC_INVALID, BC_INVALID};
            isCurrentlyInliningABlock = false;
}

VMMethod* MethodGenerationContext::Assemble() {
    // create a method instance with the given number of bytecodes and literals
    size_t numLiterals = literals.size();
    size_t numLocals = locals.size();
    VMMethod* meth = GetUniverse()->NewMethod(signature, bytecode.size(),
            numLiterals, numLocals, maxStackDepth, lexicalScope, inlinedLoops);

    // copy literals into the method
    for (size_t i = 0; i < numLiterals; i++) {
        vm_oop_t l = literals[i];
        meth->SetIndexableField(i, l);
    }

    // copy bytecodes into method
    size_t bc_size = bytecode.size();
    for (size_t i = 0; i < bc_size; i++) {
        meth->SetBytecode(i, bytecode[i]);
    }

    // return the method - the holder field is to be set later on!
    return meth;
}

VMPrimitive* MethodGenerationContext::AssemblePrimitive(bool classSide) {
    return VMPrimitive::GetEmptyPrimitive(signature, classSide);
}

MethodGenerationContext::~MethodGenerationContext() {
}

int8_t MethodGenerationContext::FindLiteralIndex(vm_oop_t lit) {
    return (int8_t)IndexOf(literals, lit);
}

uint8_t MethodGenerationContext::GetFieldIndex(VMSymbol* field) {
    int16_t idx = holderGenc.GetFieldIndex(field);
    assert(idx >= 0);
    return idx;
}

bool Contains(std::vector<Variable>& vec, std::string& name) {
    for (Variable& v : vec) {
        if (v.IsNamed(name)) {
            return true;
        }
    }

    return false;
}

size_t IndexOf(std::vector<Variable>& vec, std::string& name) {
    size_t i = 0;
    for (Variable& v : vec) {
        if (v.IsNamed(name)) {
            return i;
        }
        i += 1;
    }

    return -1;
}


bool MethodGenerationContext::FindVar(std::string& var, int64_t* index,
        int* context, bool* isArgument) {
    if ((*index = IndexOf(locals, var)) == -1) {
        if ((*index = IndexOf(arguments, var)) == -1) {
            if (!outerGenc)
                return false;
            else {
                (*context)++;
                return outerGenc->FindVar(var, index, context, isArgument);
            }
        } else
            *isArgument = true;
    }

    return true;
}

bool MethodGenerationContext::HasField(VMSymbol* field) {
    return holderGenc.HasField(field);
}

size_t MethodGenerationContext::GetNumberOfArguments() {
    return arguments.size();
}

void MethodGenerationContext::SetSignature(VMSymbol* sig) {
    signature = sig;
}

void MethodGenerationContext::SetPrimitive(bool prim) {
    primitive = prim;
}

void MethodGenerationContext::AddArgument(std::string& arg, const SourceCoordinate& coord) {
    size_t index = arguments.size();
    arguments.push_back({arg, index, true, coord});
}

void MethodGenerationContext::AddLocal(std::string& local, const SourceCoordinate& coord) {
    size_t index = locals.size();
    locals.push_back({local, index, false, coord});
}

uint8_t MethodGenerationContext::AddLiteral(vm_oop_t lit) {
    assert(!AS_OBJ(lit)->IsMarkedInvalid());

    uint8_t idx = literals.size();
    literals.push_back(lit);
    return idx;
}

int8_t MethodGenerationContext::AddLiteralIfAbsent(vm_oop_t lit) {
    int8_t idx = IndexOf(literals, lit);
    if (idx != -1) {
        assert(idx >= 0 && (size_t)idx < literals.size() && "Expect index to be inside the literals vector.");
        return idx;
    }
    return AddLiteral(lit);
}

void MethodGenerationContext::UpdateLiteral(vm_oop_t oldValue, uint8_t index, vm_oop_t newValue) {
    assert(literals.at(index) == oldValue);
    literals[index] = newValue;
}

bool MethodGenerationContext::AddArgumentIfAbsent(std::string& arg, const SourceCoordinate& coord) {
    if (Contains(locals, arg)) {
        return false;
    }
    AddArgument(arg, coord);
    return true;
}

bool MethodGenerationContext::AddLocalIfAbsent(std::string& local, const SourceCoordinate& coord) {
    if (Contains(locals, local)) {
        return false;
    }

    AddLocal(local, coord);
    return true;
}

void MethodGenerationContext::SetFinished(bool finished) {
    this->finished = finished;
}

bool MethodGenerationContext::HasBytecodes() {
    return !bytecode.empty();
}

void MethodGenerationContext::AddBytecode(uint8_t bc, size_t stackEffect) {
    currentStackDepth += stackEffect;
    maxStackDepth = max(maxStackDepth, currentStackDepth);

    bytecode.push_back(bc);

    last4Bytecodes[0] = last4Bytecodes[1];
    last4Bytecodes[1] = last4Bytecodes[2];
    last4Bytecodes[2] = last4Bytecodes[3];
    last4Bytecodes[3] = bc;
}

void MethodGenerationContext::AddBytecodeArgument(uint8_t bc) {
    bytecode.push_back(bc);
}

size_t MethodGenerationContext::AddBytecodeArgumentAndGetIndex(uint8_t bc) {
    size_t index = bytecode.size();
    AddBytecodeArgument(bc);
    return index;
}

bool MethodGenerationContext::lastBytecodeIs(size_t indexFromEnd, uint8_t bytecode) {
    assert(indexFromEnd >= 0 && indexFromEnd < 4);
    uint8_t actual = last4Bytecodes[3 - indexFromEnd];
    return actual == bytecode;
}

void MethodGenerationContext::removeLastBytecodes(size_t numBytecodes) {
    assert(numBytecodes > 0 && numBytecodes <= 4);
    size_t bytesToRemove = 0;

    for (size_t idxFromEnd = 0; idxFromEnd < numBytecodes; idxFromEnd += 1) {
        bytesToRemove += Bytecode::GetBytecodeLength(last4Bytecodes[3 - idxFromEnd]);
    }

    bytecode.erase(bytecode.end() - bytesToRemove, bytecode.end());
}

bool MethodGenerationContext::hasTwoLiteralBlockArguments() {
    if (!lastBytecodeIs(0, BC_PUSH_BLOCK)) {
        return false;
    }

    return lastBytecodeIs(1, BC_PUSH_BLOCK);
}

/**
 * This works only, because we have a simple forward-pass parser,
 * and inlining, where this is used, happens right after the block was added.
 * This also means, we need to remove blocks in reverse order.
 */
vm_oop_t MethodGenerationContext::getLastBlockMethodAndFreeLiteral(uint8_t blockLiteralIdx) {
    assert(blockLiteralIdx == literals.size() - 1);
    vm_oop_t block = literals.back();
    literals.pop_back();
    return block;
}

std::tuple<vm_oop_t, vm_oop_t> MethodGenerationContext::extractBlockMethodsAndRemoveBytecodes() {
    uint8_t block1LitIdx = bytecode.at(bytecode.size() - 3);
    uint8_t block2LitIdx = bytecode.at(bytecode.size() - 1);

    // grab the blocks' methods for inlining
    vm_oop_t toBeInlined2 = getLastBlockMethodAndFreeLiteral(block2LitIdx);
    vm_oop_t toBeInlined1 = getLastBlockMethodAndFreeLiteral(block1LitIdx);
    
    removeLastBytecodes(2);

    return {toBeInlined1, toBeInlined2};
}

bool MethodGenerationContext::InlineWhile(Parser& parser, bool isWhileTrue) {
    if (!hasTwoLiteralBlockArguments()) {
        return false;
    }

    assert(Bytecode::GetBytecodeLength(BC_PUSH_BLOCK) == 2);

    std::tuple<vm_oop_t, vm_oop_t> methods = extractBlockMethodsAndRemoveBytecodes();
    VMMethod* condMethod = static_cast<VMMethod*>(std::get<0>(methods));
    VMMethod* bodyMethod = static_cast<VMMethod*>(std::get<1>(methods));

    size_t loopBeginIdx = OffsetOfNextInstruction();

    isCurrentlyInliningABlock = true;
    condMethod->InlineInto(*this);

    size_t jumpOffsetIdxToSkipLoopBody = EmitJumpOnBoolWithDummyOffset(*this, isWhileTrue, true);

    bodyMethod->InlineInto(*this);

    completeJumpsAndEmitReturningNil(parser, loopBeginIdx, jumpOffsetIdxToSkipLoopBody);

    isCurrentlyInliningABlock = false;

    return true;
}

void MethodGenerationContext::CompleteLexicalScope() {
    lexicalScope = new LexicalScope(outerGenc == nullptr ? nullptr : outerGenc->lexicalScope,
                                    arguments, locals);
}

void MethodGenerationContext::MergeIntoScope(LexicalScope& scopeToBeInlined) {
    assert(scopeToBeInlined.GetNumberOfArguments() == 1);
    size_t numLocals = scopeToBeInlined.GetNumberOfLocals();
    if (numLocals > 0) {
        inlineLocals(scopeToBeInlined);
    }
}

void MethodGenerationContext::inlineLocals(LexicalScope& scopeToBeInlined) {
    for (const Variable& local : scopeToBeInlined.locals) {
        Variable freshCopy = local.CopyForInlining(this->locals.size());
        if (freshCopy.IsValid()) {
            // freshCopy can be invalid, because we don't need the $blockSelf
            std::string qualifiedName = local.MakeQualifiedName();
            assert(!Contains(this->locals, qualifiedName));
            lexicalScope->AddInlinedLocal(freshCopy);
            this->locals.push_back(freshCopy);
        }
    }
}

uint8_t MethodGenerationContext::GetInlinedLocalIdx(const Variable* var) const {
    assert(locals.size() < UINT8_MAX);
    uint8_t index = locals.size();

    while (index > 0) {
        index -= 1;
        if (locals[index].IsSame(*var)) {
            return index;
        }
    }

    char msg[200];
    std::string qualifiedName = var->MakeQualifiedName();
    snprintf(msg, 200, "Unexpected issue trying to find an inlined variable. %s could not be found.", qualifiedName.data());
    Universe::ErrorExit(msg);
}

void MethodGenerationContext::checkJumpOffset(size_t jumpOffset, uint8_t bytecode) {
    if (jumpOffset < 0 || jumpOffset > 0xFFFF) {
        char msg[100];
        snprintf(msg, 100, "The jumpOffset for the %s bytecode is out of range: %zu\n", Bytecode::GetBytecodeName(bytecode), jumpOffset);
        Universe::ErrorExit(msg);
    }
}

void MethodGenerationContext::EmitBackwardsJumpOffsetToTarget(size_t loopBeginIdx) {
    size_t addressOfJump = OffsetOfNextInstruction();

    // we are going to jump backward and want a positive value
    // thus we subtract target_address from address_of_jump
    size_t jumpOffset = addressOfJump - loopBeginIdx;

    checkJumpOffset(jumpOffset, BC_JUMP_BACKWARD);

    size_t backwardJumpIdx = OffsetOfNextInstruction();

    EmitJumpBackwardWithOffset(*this, jumpOffset);
    inlinedLoops.push_back(BackJump(loopBeginIdx, backwardJumpIdx));
}

void MethodGenerationContext::completeJumpsAndEmitReturningNil(Parser& parser, size_t loopBeginIdx, size_t jumpOffsetIdxToSkipLoopBody) {
    resetLastBytecodeBuffer();
    EmitPOP(*this);

    EmitBackwardsJumpOffsetToTarget(loopBeginIdx);

    PatchJumpOffsetToPointToNextInstruction(jumpOffsetIdxToSkipLoopBody);
    EmitPUSHCONSTANT(*this, load_ptr(nilObject));
    resetLastBytecodeBuffer();
}

void MethodGenerationContext::PatchJumpOffsetToPointToNextInstruction(size_t indexOfOffset) {
    size_t instructionStart = indexOfOffset - 1;
    uint8_t bytecode = this->bytecode[instructionStart];
    assert(IsJumpBytecode(bytecode));

    size_t jumpOffset = OffsetOfNextInstruction() - instructionStart;
    checkJumpOffset(jumpOffset, bytecode);

    if (jumpOffset <= 0xFF) {
        this->bytecode[indexOfOffset] = jumpOffset;
        this->bytecode[indexOfOffset + 1] = 0;
    } else {
        // need to use the JUMP2* version of the bytecode
        if (bytecode < FIRST_DOUBLE_BYTE_JUMP_BYTECODE) {
            // still need to bump this one up to
            this->bytecode[instructionStart] += NUM_SINGLE_BYTE_JUMP_BYTECODES;
        }
        assert(IsJumpBytecode(this->bytecode[instructionStart]));
        this->bytecode[indexOfOffset] = jumpOffset & 0xFF;
        this->bytecode[indexOfOffset + 1] = jumpOffset >> 8;
    }
}

void MethodGenerationContext::resetLastBytecodeBuffer() {
    last4Bytecodes[0] = BC_INVALID;
    last4Bytecodes[1] = BC_INVALID;
    last4Bytecodes[2] = BC_INVALID;
    last4Bytecodes[3] = BC_INVALID;
}
