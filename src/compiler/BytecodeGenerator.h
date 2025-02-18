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

#include "../interpreter/bytecodes.h"
#include "../misc/defs.h"
#include "../primitivesCore/PrimitiveContainer.h"
#include "../vmobjects/ObjectFormats.h"
#include "MethodGenerationContext.h"

void Emit1(MethodGenerationContext& mgenc, uint8_t bytecode,
           int64_t stackEffect);
void Emit2(MethodGenerationContext& mgenc, uint8_t bytecode, size_t idx,
           int64_t stackEffect);
void Emit3(MethodGenerationContext& mgenc, uint8_t bytecode, size_t idx,
           size_t ctx, int64_t stackEffect);

void EmitHALT(MethodGenerationContext& mgenc);
void EmitDUP(MethodGenerationContext& mgenc);
void EmitPUSHLOCAL(MethodGenerationContext& mgenc, size_t idx, size_t ctx);
void EmitPUSHARGUMENT(MethodGenerationContext& mgenc, size_t idx, size_t ctx);
void EmitPUSHFIELD(MethodGenerationContext& mgenc, VMSymbol* field);
void EmitPUSHBLOCK(MethodGenerationContext& mgenc, VMInvokable* block);
void EmitPUSHCONSTANT(MethodGenerationContext& mgenc, vm_oop_t cst);
void EmitPUSHCONSTANT(MethodGenerationContext& mgenc, uint8_t literalIndex);
void EmitPUSHCONSTANTString(MethodGenerationContext& mgenc, VMString* str);
void EmitPUSHGLOBAL(MethodGenerationContext& mgenc, VMSymbol* global);
void EmitPOP(MethodGenerationContext& mgenc);
void EmitPOPLOCAL(MethodGenerationContext& mgenc, size_t idx, size_t ctx);
void EmitPOPARGUMENT(MethodGenerationContext& mgenc, size_t idx, size_t ctx);
void EmitPOPFIELD(MethodGenerationContext& mgenc, VMSymbol* field);
void EmitSEND(MethodGenerationContext& mgenc, VMSymbol* msg);
void EmitSUPERSEND(MethodGenerationContext& mgenc, VMSymbol* msg);
void EmitRETURNSELF(MethodGenerationContext& mgenc);
void EmitRETURNLOCAL(MethodGenerationContext& mgenc);
void EmitRETURNNONLOCAL(MethodGenerationContext& mgenc);
void EmitRETURNFIELD(MethodGenerationContext& mgenc, size_t index);

void EmitINC(MethodGenerationContext& mgenc);
void EmitDEC(MethodGenerationContext& mgenc);
void EmitIncFieldPush(MethodGenerationContext& mgenc, uint8_t fieldIdx);

void EmitDupSecond(MethodGenerationContext& mgenc);

size_t EmitJumpOnWithDummyOffset(MethodGenerationContext& mgenc,
                                 JumpCondition condition, bool needsPop);
size_t EmitJumpWithDumyOffset(MethodGenerationContext& mgenc);
size_t EmitJumpIfGreaterWithDummyOffset(MethodGenerationContext& mgenc);
void EmitJumpBackwardWithOffset(MethodGenerationContext& mgenc,
                                size_t jumpOffset);
size_t Emit3WithDummy(MethodGenerationContext& mgenc, uint8_t bytecode,
                      int64_t stackEffect);

void EmitPushFieldWithIndex(MethodGenerationContext& mgenc, uint8_t fieldIdx);
void EmitPopFieldWithIndex(MethodGenerationContext& mgenc, uint8_t fieldIdx);
