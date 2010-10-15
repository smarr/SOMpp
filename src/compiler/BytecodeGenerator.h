#pragma once
#ifndef BYTECODEGENERATOR_H_
#define BYTECODEGENERATOR_H_

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

#include "../misc/defs.h"

class VMMethod;
class VMSymbol;
class VMObject;
class VMString;

class BytecodeGenerator {
public:
	void EmitHALT(MethodGenerationContext* mgenc);
	void EmitDUP(MethodGenerationContext* mgenc);
	void EmitPUSHLOCAL(MethodGenerationContext* mgenc, int idx, int ctx);
	void EmitPUSHARGUMENT(MethodGenerationContext* mgenc, int idx, int ctx);
	void EmitPUSHFIELD(MethodGenerationContext* mgenc, pVMSymbol field);
	void EmitPUSHBLOCK(MethodGenerationContext* mgenc, pVMMethod block);
	void EmitPUSHCONSTANT(MethodGenerationContext* mgenc, pVMObject cst);
	void EmitPUSHCONSTANTString(
        MethodGenerationContext* mgenc, pVMString str);
	void EmitPUSHGLOBAL(MethodGenerationContext* mgenc, pVMSymbol global);
	void EmitPOP(MethodGenerationContext* mgenc);
	void EmitPOPLOCAL(MethodGenerationContext* mgenc, int idx, int ctx);
	void EmitPOPARGUMENT(MethodGenerationContext* mgenc, int idx, int ctx);
	void EmitPOPFIELD(MethodGenerationContext* mgenc, pVMSymbol field);
	void EmitSEND(MethodGenerationContext* mgenc, pVMSymbol msg);
	void EmitSUPERSEND(MethodGenerationContext* mgenc, pVMSymbol msg);
	void EmitRETURNLOCAL(MethodGenerationContext* mgenc);
	void EmitRETURNNONLOCAL(MethodGenerationContext* mgenc);
};

#endif
