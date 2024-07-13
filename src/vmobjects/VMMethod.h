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

#include <iostream>
#include <queue>

#include "../compiler/LexicalScope.h"
#include "../vm/Globals.h"
#include "../vm/Print.h"
#include "VMInteger.h"
#include "VMInvokable.h"

class MethodGenerationContext;
class Interpreter;

class Jump {
public:
    Jump(size_t originalJumpTargetIdx, uint8_t jumpBc, size_t idx) : originalJumpTargetIdx(originalJumpTargetIdx), jumpBc(jumpBc), idx(idx) {}

//    Jump(Jump&& jmp) = default;
//    Jump(const Jump& jmp) = default;

    size_t  originalJumpTargetIdx; // order by
    uint8_t jumpBc;
    size_t  idx;
};

bool operator<(const Jump& a, const Jump& b);


class BackJump {
public:
    BackJump(size_t loopBeginIdx, size_t backwardJumpIdx) : loopBeginIdx(loopBeginIdx), backwardJumpIdx(backwardJumpIdx) {}

    BackJump(const BackJump& jmp) = default;

    BackJump() : loopBeginIdx(-1), backwardJumpIdx(-1) {}

    size_t loopBeginIdx; // order by
    size_t backwardJumpIdx;

    bool IsValid() const {
        return loopBeginIdx != -1;
    }
};

bool operator<(const BackJump& a, const BackJump& b);


class BackJumpPatch {
public:
    BackJumpPatch(size_t backwardsJumpIdx, size_t loopBeginIdx) : backwardsJumpIdx(backwardsJumpIdx), loopBeginIdx(loopBeginIdx) {}

    BackJumpPatch(const BackJumpPatch& patch) = default;

    size_t backwardsJumpIdx; // order by
    size_t loopBeginIdx;
};

bool operator<(const BackJumpPatch& a, const BackJumpPatch& b);


class VMMethod: public VMInvokable {
    friend class Interpreter;
    friend class Disassembler;

public:
    typedef GCMethod Stored;

    VMMethod(VMSymbol* signature, size_t bcCount, size_t numberOfConstants, size_t numLocals, size_t maxStackDepth, LexicalScope* lexicalScope, BackJump* inlinedLoops);

    ~VMMethod() override {
        delete lexicalScope;
    }

    inline size_t GetNumberOfLocals() const {
        return numberOfLocals;
    }

    VMClass* GetClass() const override {
        return load_ptr(methodClass);
    }

    inline size_t GetObjectSize() const override {
        size_t additionalBytes = PADDED_SIZE(bcLength + numberOfConstants*sizeof(VMObject*));
        return additionalBytes + sizeof(VMMethod);
    }

    size_t GetMaximumNumberOfStackElements() const {
        return maximumNumberOfStackElements;
    }

    inline size_t GetNumberOfArguments() const {
        return numberOfArguments;
    }

    size_t GetNumberOfBytecodes() const {
        return bcLength;
    }
            void      SetHolder(VMClass* hld) override;
            void      SetHolderAll(VMClass* hld);

            inline vm_oop_t GetConstant(size_t bytecodeIndex) const {
                const uint8_t bc = bytecodes[bytecodeIndex + 1];
                if (bc >= GetNumberOfIndexableFields()) {
                    ErrorPrint("Error: Constant index out of range\n");
                    return nullptr;
                }
                return GetIndexableField(bc);
            }

    inline  uint8_t   GetBytecode(long indx) const {
        return bytecodes[indx];
    }

    inline  void      SetBytecode(long indx, uint8_t val) {
        bytecodes[indx] = val;
    }

#ifdef UNSAFE_FRAME_OPTIMIZATION
    void SetCachedFrame(VMFrame* frame);
    VMFrame* GetCachedFrame() const;
#endif

    void WalkObjects(walk_heap_fn) override;

    inline  size_t GetNumberOfIndexableFields() const {
        return numberOfConstants;
    }

    VMMethod* CloneForMovingGC() const override;

    inline  void SetIndexableField(long idx, vm_oop_t item) {
        store_ptr(indexableFields[idx], item);
    }

    void Invoke(Interpreter* interp, VMFrame* frame) override;

    void MarkObjectAsInvalid() override {
        indexableFields = (gc_oop_t*) INVALID_GC_POINTER;
    }

    bool IsMarkedInvalid() const override {
        return indexableFields == (gc_oop_t*) INVALID_GC_POINTER;
    }

    StdString AsDebugString() const override;

    void InlineInto(MethodGenerationContext& mgenc);

    void AdaptAfterOuterInlined(uint8_t removedCtxLevel, MethodGenerationContext& mgencWithInlined);

private:
    void inlineInto(MethodGenerationContext& mgenc);
    std::priority_queue<BackJump> createBackJumpHeap();

    inline uint8_t* GetBytecodes() const {
        return bytecodes;
    }

    inline vm_oop_t GetIndexableField(long idx) const {
        return load_ptr(indexableFields[idx]);
    }

    static void prepareBackJumpToCurrentAddress(std::priority_queue<BackJump>& backJumps, std::priority_queue<BackJumpPatch>& backJumpsToPatch, size_t i, MethodGenerationContext& mgenc);

    static void patchJumpToCurrentAddress(size_t i, std::priority_queue<Jump>& jumps, MethodGenerationContext& mgenc);

private_testable:
    const size_t numberOfLocals;
    const size_t maximumNumberOfStackElements;
    const size_t bcLength;
    const size_t numberOfArguments;
    const size_t numberOfConstants;

private:
    LexicalScope* lexicalScope;
    BackJump*    inlinedLoops;

#ifdef UNSAFE_FRAME_OPTIMIZATION
    GCFrame* cachedFrame;
#endif
    gc_oop_t* indexableFields;
    uint8_t* bytecodes;
};
