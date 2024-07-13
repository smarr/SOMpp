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

#include <misc/defs.h>

#include "ClassGenerationContext.h"

#include <vmobjects/ObjectFormats.h>


class MethodGenerationContext {
public:
    MethodGenerationContext();
    ~MethodGenerationContext();

    VMMethod* Assemble();
    VMPrimitive* AssemblePrimitive(bool classSide);

    int8_t FindLiteralIndex(vm_oop_t lit);
    bool FindVar(VMSymbol* var, size_t* index,
            int* context, bool* isArgument);
    bool HasField(VMSymbol* field);
    
    uint8_t GetFieldIndex(VMSymbol* field);

    void SetHolder(ClassGenerationContext* holder);
    void SetOuter(MethodGenerationContext* outer);
    void SetIsBlockMethod(bool isBlock = true);
    void SetSignature(VMSymbol* sig);
    void AddArgument(const StdString& arg);
    void SetPrimitive(bool prim = true);
    void AddLocal(const StdString& local);
    uint8_t AddLiteral(vm_oop_t lit);
    void UpdateLiteral(vm_oop_t oldValue, uint8_t index, vm_oop_t newValue);
    bool AddArgumentIfAbsent(const StdString& arg);
    bool AddLocalIfAbsent(const StdString& local);
    int8_t AddLiteralIfAbsent(vm_oop_t lit);
    void SetFinished(bool finished = true);

    ClassGenerationContext* GetHolder();
    MethodGenerationContext* GetOuter();

    VMSymbol* GetSignature();
    bool IsPrimitive();
    bool IsBlockMethod();
    bool IsFinished();
    void RemoveLastBytecode() {bytecode.pop_back();};
    size_t GetNumberOfArguments();
    size_t AddBytecode(uint8_t bc, size_t stackEffect);
    size_t AddBytecodeArgument(uint8_t bc);

    bool HasBytecodes();
    std::vector<uint8_t> GetBytecodes() {
        return bytecode;
    }

private:
    ClassGenerationContext* holderGenc;
    MethodGenerationContext* outerGenc;
    bool blockMethod;
    VMSymbol* signature;
    std::vector<VMSymbol*> arguments;
    bool primitive;
    std::vector<VMSymbol*> locals;
    std::vector<vm_oop_t> literals;
    bool finished;
    std::vector<uint8_t> bytecode;
    
    size_t currentStackDepth;
    size_t maxStackDepth;
};
