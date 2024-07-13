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

#include <assert.h>

#include "BytecodeGenerator.h"
#include <vm/Universe.h>

#include <vmobjects/VMObject.h>
#include <vmobjects/VMMethod.h>
#include <vmobjects/VMSymbol.h>
#include <vmobjects/Signature.h>

#define EMIT1(BC, stackEffect) \
    mgenc->AddBytecode(BC, stackEffect)

#define EMIT2(BC, IDX, stackEffect) \
    mgenc->AddBytecode(BC, stackEffect);\
	mgenc->AddBytecodeArgument(IDX)

#define EMIT3(BC, IDX, CTX, stackEffect) \
    mgenc->AddBytecode(BC, stackEffect);\
	mgenc->AddBytecodeArgument(IDX);\
	mgenc->AddBytecodeArgument(CTX)

void BytecodeGenerator::EmitHALT(MethodGenerationContext* mgenc) {
    EMIT1(BC_HALT, 0);
}

void BytecodeGenerator::EmitDUP(MethodGenerationContext* mgenc) {
    EMIT1(BC_DUP, 1);
}

void BytecodeGenerator::EmitPUSHLOCAL(MethodGenerationContext* mgenc, long idx,
        int ctx) {
    assert(idx >= 0);
    assert(ctx >= 0);
    if (ctx == 0) {
        if (idx == 0) {
            EMIT1(BC_PUSH_LOCAL_0, 1);
            return;
        }
        if (idx == 1) {
            EMIT1(BC_PUSH_LOCAL_1, 1);
            return;
        }
        if (idx == 2) {
            EMIT1(BC_PUSH_LOCAL_2, 1);
            return;
        }
    }
    EMIT3(BC_PUSH_LOCAL, idx, ctx, 1);
}

void BytecodeGenerator::EmitPUSHARGUMENT(MethodGenerationContext* mgenc,
        long idx, int ctx) {
    assert(idx >= 0);
    assert(ctx >= 0);
    
    if (ctx == 0) {
        if (idx == 0) {
            EMIT1(BC_PUSH_SELF, 1);
            return;
        }
        
        if (idx == 1) {
            EMIT1(BC_PUSH_ARG_1, 1);
            return;
        }
        
        if (idx == 2) {
            EMIT1(BC_PUSH_ARG_2, 1);
            return;
        }
    }
    EMIT3(BC_PUSH_ARGUMENT, idx, ctx, 1);
}

void BytecodeGenerator::EmitPUSHFIELD(MethodGenerationContext* mgenc, VMSymbol* field) {
    uint8_t idx = mgenc->GetFieldIndex(field);
    if (idx == 0) {
        EMIT1(BC_PUSH_FIELD_0, 1);
    } else if (idx == 1) {
        EMIT1(BC_PUSH_FIELD_1, 1);
    } else {
        EMIT2(BC_PUSH_FIELD, idx, 1);
    }
}

void BytecodeGenerator::EmitPUSHBLOCK(MethodGenerationContext* mgenc, VMMethod* block) {
    int8_t idx = mgenc->AddLiteralIfAbsent(block);
    EMIT2(BC_PUSH_BLOCK, idx, 1);
}

void BytecodeGenerator::EmitPUSHCONSTANT(MethodGenerationContext* mgenc, vm_oop_t cst) {
    if (CLASS_OF(cst) == load_ptr(integerClass)) {
        if (INT_VAL(cst) == 0ll) {
            EMIT1(BC_PUSH_0, 1);
            return;
        }
        if (INT_VAL(cst) == 1ll) {
            EMIT1(BC_PUSH_1, 1);
            return;
        }
    }
    
    if (cst == load_ptr(nilObject)) {
        EMIT1(BC_PUSH_NIL, 1);
        return;
    }
    
    int8_t idx = mgenc->AddLiteralIfAbsent(cst);
    if (idx == 0) {
        EMIT1(BC_PUSH_CONSTANT_0, 1);
        return;
    }
    if (idx == 1) {
        EMIT1(BC_PUSH_CONSTANT_1, 1);
        return;
    }
    if (idx == 2) {
        EMIT1(BC_PUSH_CONSTANT_2, 1);
        return;
    }
    
    EMIT2(BC_PUSH_CONSTANT, idx, 1);
}

void BytecodeGenerator::EmitPUSHCONSTANT(MethodGenerationContext* mgenc,
        uint8_t literalIndex) {
    EMIT2(BC_PUSH_CONSTANT, literalIndex, 1);
}

void BytecodeGenerator::EmitPUSHCONSTANTString(MethodGenerationContext* mgenc,
        VMString* str) {
    EMIT2(BC_PUSH_CONSTANT, mgenc->FindLiteralIndex(str), 1);
}

void BytecodeGenerator::EmitPUSHGLOBAL(MethodGenerationContext* mgenc, VMSymbol* global) {
    if (global == GetUniverse()->SymbolFor("nil")) {
        EmitPUSHCONSTANT(mgenc, load_ptr(nilObject));
    } else if (global == GetUniverse()->SymbolFor("true")) {
        EmitPUSHCONSTANT(mgenc, load_ptr(trueObject));
    } else if (global == GetUniverse()->SymbolFor("false")) {
        EmitPUSHCONSTANT(mgenc, load_ptr(falseObject));
    } else {
        int8_t idx = mgenc->AddLiteralIfAbsent(global);
        EMIT2(BC_PUSH_GLOBAL, idx, 1);
    }
}

void BytecodeGenerator::EmitPOP(MethodGenerationContext* mgenc) {
    EMIT1(BC_POP, -1);
}

void BytecodeGenerator::EmitPOPLOCAL(MethodGenerationContext* mgenc, long idx,
        int ctx) {
    assert(idx >= 0);
    assert(ctx >= 0);
    if (ctx == 0) {
        if (idx == 0) {
            EMIT1(BC_POP_LOCAL_0, -1);
            return;
        }
        
        if (idx == 1) {
            EMIT1(BC_POP_LOCAL_1, -1);
            return;
        }
        
        if (idx == 2) {
            EMIT1(BC_POP_LOCAL_2, -1);
            return;
        }
    }
    
    EMIT3(BC_POP_LOCAL, idx, ctx, -1);
}

void BytecodeGenerator::EmitPOPARGUMENT(MethodGenerationContext* mgenc,
                                        long idx, int ctx) {
    EMIT3(BC_POP_ARGUMENT, idx, ctx, -1);
}

void BytecodeGenerator::EmitPOPFIELD(MethodGenerationContext* mgenc, VMSymbol* field) {
    uint8_t idx = mgenc->GetFieldIndex(field);
    
    if (idx == 0) {
        EMIT1(BC_POP_FIELD_0, -1);
    } else if (idx == 1) {
        EMIT1(BC_POP_FIELD_1, -1);
    } else {
        EMIT2(BC_POP_FIELD, idx, -1);
    }
}

void BytecodeGenerator::EmitSEND(MethodGenerationContext* mgenc, VMSymbol* msg) {
    int8_t idx = mgenc->AddLiteralIfAbsent(msg);
    
    int numArgs = Signature::GetNumberOfArguments(msg);
    size_t stackEffect = -numArgs + 1;  // +1 for the result
    
    EMIT2(BC_SEND, idx, stackEffect);
}

void BytecodeGenerator::EmitSUPERSEND(MethodGenerationContext* mgenc, VMSymbol* msg) {
    int8_t idx = mgenc->AddLiteralIfAbsent(msg);
    
    int numArgs = Signature::GetNumberOfArguments(msg);
    size_t stackEffect = -numArgs + 1;  // +1 for the result
    
    EMIT2(BC_SUPER_SEND, idx, stackEffect);
}

void BytecodeGenerator::EmitRETURNLOCAL(MethodGenerationContext* mgenc) {
    EMIT1(BC_RETURN_LOCAL, 0);
}

void BytecodeGenerator::EmitRETURNNONLOCAL(MethodGenerationContext* mgenc) {
    EMIT1(BC_RETURN_NON_LOCAL, 0);
}
