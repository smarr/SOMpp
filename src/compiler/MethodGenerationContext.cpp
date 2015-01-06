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

#include "MethodGenerationContext.h"

#include "../interpreter/bytecodes.h"

#include "../vmobjects/VMSymbol.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/Signature.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMPrimitive.h"

MethodGenerationContext::MethodGenerationContext() {
    //signature = 0;
    holderGenc = 0;
    outerGenc = 0;
    arguments.Clear();
    literals.Clear();
    locals.Clear();
    bytecode.clear();
    primitive = false;
    blockMethod = false;
    finished = false;
}

VMMethod* MethodGenerationContext::Assemble() {
    // create a method instance with the given number of bytecodes and literals
    size_t numLiterals = literals.Size();

    VMMethod* meth = GetUniverse()->NewMethod(signature, bytecode.size(),
            numLiterals);

    // populate the fields that are immediately available
    size_t numLocals = locals.Size();
    meth->SetNumberOfLocals(numLocals);

    meth->SetMaximumNumberOfStackElements(ComputeStackDepth());

    // copy literals into the method
    for (int i = 0; i < numLiterals; i++) {
        vm_oop_t l = literals.Get(i);
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
    return (int8_t)literals.IndexOf(lit);
}

uint8_t MethodGenerationContext::GetFieldIndex(VMSymbol* field) {
    int16_t idx = holderGenc->GetFieldIndex(field);
    assert(idx >= 0);
    return idx;
}

bool MethodGenerationContext::FindVar(const StdString& var, size_t* index,
        int* context, bool* isArgument) {
    if ((*index = locals.IndexOf(var)) == -1) {
        if ((*index = arguments.IndexOf(var)) == -1) {
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

bool MethodGenerationContext::HasField(const StdString& field) {
    return holderGenc->HasField(field);
}

size_t MethodGenerationContext::GetNumberOfArguments() {
    return arguments.Size();
}

uint8_t MethodGenerationContext::ComputeStackDepth() {
    uint8_t depth = 0;
    uint8_t maxDepth = 0;
    unsigned int i = 0;

    while (i < bytecode.size()) {
        switch (bytecode[i]) {
        case BC_HALT:
            i++;
            break;
        case BC_DUP:
            depth++;
            i++;
            break;
        case BC_PUSH_LOCAL:
        case BC_PUSH_ARGUMENT:
            depth++;
            i += 3;
            break;
        case BC_PUSH_FIELD:
        case BC_PUSH_BLOCK:
        case BC_PUSH_CONSTANT:
        case BC_PUSH_GLOBAL:
            depth++;
            i += 2;
            break;
        case BC_POP:
            depth--;
            i++;
            break;
        case BC_POP_LOCAL:
        case BC_POP_ARGUMENT:
            depth--;
            i += 3;
            break;
        case BC_POP_FIELD:
            depth--;
            i += 2;
            break;
        case BC_SEND:
        case BC_SUPER_SEND: {
            // these are special: they need to look at the number of
            // arguments (extractable from the signature)
            VMSymbol* sig = static_cast<VMSymbol*>(literals.Get(bytecode[i + 1]));

            depth -= Signature::GetNumberOfArguments(sig);

            depth++; // return value
            i += 2;
            break;
        }
        case BC_RETURN_LOCAL:
        case BC_RETURN_NON_LOCAL: {
            i++;
            break;
        }
        case BC_JUMP_IF_FALSE:
        case BC_JUMP_IF_TRUE:
            depth--;
            i += 5;
            break;
        case BC_JUMP:
            i += 5;
            break;
        default: {
            Universe::ErrorPrint("Illegal bytecode: " + to_string(bytecode[i]) + "\n");
            GetUniverse()->Quit(1);
          }
        }

        if (depth > maxDepth)
            maxDepth = depth;
    }

    return maxDepth;
}

void MethodGenerationContext::SetHolder(ClassGenerationContext* holder) {
    holderGenc = holder;
}

void MethodGenerationContext::SetOuter(MethodGenerationContext* outer) {
    outerGenc = outer;
}

void MethodGenerationContext::SetIsBlockMethod(bool isBlock) {
    blockMethod = isBlock;
}

void MethodGenerationContext::SetSignature(VMSymbol* sig) {
    signature = sig;
}

void MethodGenerationContext::SetPrimitive(bool prim) {
    primitive = prim;
}

void MethodGenerationContext::AddArgument(const StdString& arg) {
    arguments.PushBack(arg);
}

void MethodGenerationContext::AddLocal(const StdString& local) {
    locals.PushBack(local);
}

void MethodGenerationContext::AddLiteral(vm_oop_t lit) {
    literals.PushBack(lit);
}

bool MethodGenerationContext::AddArgumentIfAbsent(const StdString& arg) {
    if (locals.IndexOf(arg) != -1)
        return false;
    arguments.PushBack(arg);
    return true;
}

bool MethodGenerationContext::AddLocalIfAbsent(const StdString& local) {
    if (locals.IndexOf(local) != -1)
        return false;
    locals.PushBack(local);
    return true;
}

bool MethodGenerationContext::AddLiteralIfAbsent(vm_oop_t lit) {
    if (literals.IndexOf(lit) != -1) return false;
    literals.PushBack(lit);
    return true;
}
void MethodGenerationContext::SetFinished(bool finished) {
    this->finished = finished;
}

ClassGenerationContext* MethodGenerationContext::GetHolder() {
    return holderGenc;
}

MethodGenerationContext* MethodGenerationContext::GetOuter() {
    return outerGenc;
}

VMSymbol* MethodGenerationContext::GetSignature() {
    return signature;
}

bool MethodGenerationContext::IsPrimitive() {
    return primitive;
}

bool MethodGenerationContext::IsBlockMethod() {
    return blockMethod;
}

bool MethodGenerationContext::IsFinished() {
    return finished;
}

size_t MethodGenerationContext::AddBytecode(uint8_t bc) {
    bytecode.push_back(bc);
    return bytecode.size();
}

void MethodGenerationContext::PatchJumpTarget(size_t jumpPosition) {
    size_t jump_target = bytecode.size();
    bytecode[jumpPosition]     = (uint8_t) jump_target;
    bytecode[jumpPosition + 1] = (uint8_t) (jump_target >> 8);
    bytecode[jumpPosition + 2] = (uint8_t) (jump_target >> 16);
    bytecode[jumpPosition + 3] = (uint8_t) (jump_target >> 24);
}
