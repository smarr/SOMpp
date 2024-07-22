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
#include <string>
#include <vector>

#include "../misc/VectorUtil.h"
#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMPrimitive.h"
#include "../vmobjects/VMSymbol.h"
#include "ClassGenerationContext.h"
#include "LexicalScope.h"
#include "MethodGenerationContext.h"
#include "SourceCoordinate.h"
#include "Variable.h"

MethodGenerationContext::MethodGenerationContext(ClassGenerationContext& holder, MethodGenerationContext* outer) :
        holderGenc(holder), outerGenc(outer),
        blockMethod(outer != nullptr), signature(nullptr), primitive(false), finished(false),
        currentStackDepth(0), maxStackDepth(0), maxContextLevel(outer == nullptr ? 0 : outer->GetMaxContextLevel() + 1) { }

VMMethod* MethodGenerationContext::Assemble() {
    // create a method instance with the given number of bytecodes and literals
    size_t numLiterals = literals.size();
    size_t numLocals = locals.size();
    VMMethod* meth = GetUniverse()->NewMethod(signature, bytecode.size(),
            numLiterals, numLocals, maxStackDepth, lexicalScope);

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
}

void MethodGenerationContext::AddBytecodeArgument(uint8_t bc) {
    bytecode.push_back(bc);
}

void MethodGenerationContext::CompleteLexicalScope() {
    lexicalScope = new LexicalScope(outerGenc == nullptr ? nullptr : outerGenc->lexicalScope,
                                    arguments, locals);
}
