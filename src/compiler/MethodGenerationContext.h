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
#include <misc/ExtendedList.h>

#include "ClassGenerationContext.h"

#include <vmobjects/ObjectFormats.h>


class MethodGenerationContext {
public:
    MethodGenerationContext();
    ~MethodGenerationContext();

    VMMethod* Assemble();
    VMPrimitive* AssemblePrimitive(bool classSide);

    int8_t FindLiteralIndex(vm_oop_t lit);
    bool FindVar(const StdString& var, size_t* index,
            int* context, bool* isArgument);
    bool HasField(const StdString& field);
    uint8_t ComputeStackDepth();
    
    uint8_t GetFieldIndex(VMSymbol* field);

    void SetHolder(ClassGenerationContext* holder);
    void SetOuter(MethodGenerationContext* outer);
    void SetIsBlockMethod(bool isBlock = true);
    void SetSignature(VMSymbol* sig);
    void AddArgument(const StdString& arg);
    void SetPrimitive(bool prim = true);
    void AddLocal(const StdString& local);
    void AddLiteral(vm_oop_t lit);
    bool AddArgumentIfAbsent(const StdString& arg);
    bool AddLocalIfAbsent(const StdString& local);
    bool AddLiteralIfAbsent(vm_oop_t lit);
    void SetFinished(bool finished = true);

    ClassGenerationContext* GetHolder();
    MethodGenerationContext* GetOuter();

    VMSymbol* GetSignature();
    bool IsPrimitive();
    bool IsBlockMethod();
    bool IsFinished();
    void RemoveLastBytecode() {bytecode.pop_back();};
    size_t GetNumberOfArguments();
    size_t AddBytecode(uint8_t bc);
    void PatchJumpTarget(size_t jump_position);

private:
    ClassGenerationContext* holderGenc;
    MethodGenerationContext* outerGenc;
    bool blockMethod;
    VMSymbol* signature;
    ExtendedList<StdString> arguments;
    bool primitive;
    ExtendedList<StdString> locals;
    ExtendedList<vm_oop_t> literals;
    bool finished;
    std::vector<uint8_t> bytecode;
};
