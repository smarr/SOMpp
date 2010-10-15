#pragma once
#ifndef METHODGENERATIONCONTEXT_H_
#define METHODGENERATIONCONTEXT_H_

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
#include "../misc/ExtendedList.h"

#include "ClassGenerationContext.h"

class VMMethod;
class VMArray;
class VMPrimitive;

class MethodGenerationContext {
public:
	MethodGenerationContext();
	~MethodGenerationContext();
    
    pVMMethod       Assemble();
    pVMPrimitive    AssemblePrimitive();

	int8_t          FindLiteralIndex(pVMObject lit);
	bool            FindVar(const StdString& var, int* index, 
                            int* context, bool* isArgument);
	bool            FindField(const StdString& field);
	uint8_t         ComputeStackDepth();

	void            SetHolder(ClassGenerationContext* holder);
	void            SetOuter(MethodGenerationContext* outer);
	void            SetIsBlockMethod(bool isBlock = true);
	void            SetSignature(pVMSymbol sig);
	void            AddArgument(const StdString& arg);
	void            SetPrimitive(bool prim = true);
	void            AddLocal(const StdString& local);
	void            AddLiteral(pVMObject lit);
	bool            AddArgumentIfAbsent(const StdString& arg);
	bool            AddLocalIfAbsent(const StdString& local);
	bool            AddLiteralIfAbsent(pVMObject lit);
	void            SetFinished(bool finished = true);

	ClassGenerationContext*     GetHolder();
	MethodGenerationContext*    GetOuter();

	pVMSymbol       GetSignature();
	bool            IsPrimitive();
	bool            IsBlockMethod();
	bool            IsFinished();
	void            RemoveLastBytecode() { bytecode.pop_back(); };
	int             GetNumberOfArguments();
	void            AddBytecode(uint8_t bc);
private:
	ClassGenerationContext*    holderGenc;
    MethodGenerationContext*   outerGenc;
    bool                       blockMethod;
    pVMSymbol                  signature;
    ExtendedList<StdString>    arguments;
    bool                       primitive;
    ExtendedList<StdString>    locals;
    ExtendedList<pVMObject>    literals;
    bool                       finished;
    std::vector<uint8_t>            bytecode;
};

#endif
