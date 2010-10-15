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

#include "BytecodeGenerator.h"

#include "../vmobjects/VMObject.h"
#include "../vmobjects/VMMethod.h"


#define EMIT1(BC) \
    mgenc->AddBytecode(BC)


#define EMIT2(BC, IDX) \
    mgenc->AddBytecode(BC);\
	mgenc->AddBytecode(IDX)


#define EMIT3(BC, IDX, CTX) \
    mgenc->AddBytecode(BC);\
	mgenc->AddBytecode(IDX);\
	mgenc->AddBytecode(CTX)


void BytecodeGenerator::EmitHALT( MethodGenerationContext* mgenc ) {
    EMIT1(BC_HALT);
}


void BytecodeGenerator::EmitDUP( MethodGenerationContext* mgenc ) {
    EMIT1(BC_DUP);
}


void BytecodeGenerator::EmitPUSHLOCAL(
                MethodGenerationContext* mgenc, int idx, int ctx ) {
    EMIT3(BC_PUSH_LOCAL, idx, ctx);
}


void BytecodeGenerator::EmitPUSHARGUMENT( 
                MethodGenerationContext* mgenc, int idx, int ctx ) {
    EMIT3(BC_PUSH_ARGUMENT, idx, ctx);
}


void BytecodeGenerator::EmitPUSHFIELD(
                MethodGenerationContext* mgenc, pVMSymbol field ) {
    EMIT2(BC_PUSH_FIELD, mgenc->FindLiteralIndex((pVMObject)field));
}


void BytecodeGenerator::EmitPUSHBLOCK(
                MethodGenerationContext* mgenc, pVMMethod block ) {
    EMIT2(BC_PUSH_BLOCK, mgenc->FindLiteralIndex((pVMObject)(block)));
}


void BytecodeGenerator::EmitPUSHCONSTANT(
                MethodGenerationContext* mgenc, pVMObject cst ) {
    EMIT2(BC_PUSH_CONSTANT, mgenc->FindLiteralIndex(cst));
}


void BytecodeGenerator::EmitPUSHCONSTANTString( 
                MethodGenerationContext* mgenc, pVMString str ){
   EMIT2(BC_PUSH_CONSTANT, mgenc->FindLiteralIndex((pVMObject)str));
}


void BytecodeGenerator::EmitPUSHGLOBAL(
                MethodGenerationContext* mgenc, pVMSymbol global ) {
    EMIT2(BC_PUSH_GLOBAL, mgenc->FindLiteralIndex((pVMObject)global));
}


void BytecodeGenerator::EmitPOP( MethodGenerationContext* mgenc ) {
    EMIT1(BC_POP);
}


void BytecodeGenerator::EmitPOPLOCAL(
                MethodGenerationContext* mgenc, int idx, int ctx ) {
    EMIT3(BC_POP_LOCAL, idx, ctx);
}


void BytecodeGenerator::EmitPOPARGUMENT(
                MethodGenerationContext* mgenc, int idx, int ctx ) {
    EMIT3(BC_POP_ARGUMENT, idx, ctx);
}


void BytecodeGenerator::EmitPOPFIELD(
                MethodGenerationContext* mgenc, pVMSymbol field ) {
    EMIT2(BC_POP_FIELD, mgenc->FindLiteralIndex((pVMObject)field));
}


void BytecodeGenerator::EmitSEND(
                MethodGenerationContext* mgenc, pVMSymbol msg ) {
    EMIT2(BC_SEND, mgenc->FindLiteralIndex((pVMObject)msg));
}


void BytecodeGenerator::EmitSUPERSEND(
                MethodGenerationContext* mgenc, pVMSymbol msg ) {
    EMIT2(BC_SUPER_SEND, mgenc->FindLiteralIndex((pVMObject)msg));
}


void BytecodeGenerator::EmitRETURNLOCAL(
                MethodGenerationContext* mgenc ) {
    EMIT1(BC_RETURN_LOCAL);
}


void BytecodeGenerator::EmitRETURNNONLOCAL(
                MethodGenerationContext* mgenc ) {
    EMIT1(BC_RETURN_NON_LOCAL);
}
