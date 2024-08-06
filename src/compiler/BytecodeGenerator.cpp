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

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "../interpreter/bytecodes.h"
#include "../vm/Globals.h"
#include "../vm/IsValidObject.h"
#include "../vm/Symbols.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/Signature.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMSymbol.h"

void Emit1(MethodGenerationContext& mgenc, uint8_t bytecode,
           size_t stackEffect) {
    mgenc.AddBytecode(bytecode, stackEffect);
}

void Emit2(MethodGenerationContext& mgenc, uint8_t bytecode, size_t idx,
           size_t stackEffect) {
    mgenc.AddBytecode(bytecode, stackEffect);
    mgenc.AddBytecodeArgument(idx);
}

void Emit3(MethodGenerationContext& mgenc, uint8_t bytecode, size_t idx,
           size_t ctx, size_t stackEffect) {
    mgenc.AddBytecode(bytecode, stackEffect);
    mgenc.AddBytecodeArgument(idx);
    mgenc.AddBytecodeArgument(ctx);
}

void EmitHALT(MethodGenerationContext& mgenc) {
    Emit1(mgenc, BC_HALT, 0);
}

void EmitDUP(MethodGenerationContext& mgenc) {
    Emit1(mgenc, BC_DUP, 1);
}

void EmitPUSHLOCAL(MethodGenerationContext& mgenc, long idx, int ctx) {
    assert(idx >= 0);
    assert(ctx >= 0);
    if (ctx == 0) {
        if (idx == 0) {
            Emit1(mgenc, BC_PUSH_LOCAL_0, 1);
            return;
        }
        if (idx == 1) {
            Emit1(mgenc, BC_PUSH_LOCAL_1, 1);
            return;
        }
        if (idx == 2) {
            Emit1(mgenc, BC_PUSH_LOCAL_2, 1);
            return;
        }
    }
    Emit3(mgenc, BC_PUSH_LOCAL, idx, ctx, 1);
}

void EmitPUSHARGUMENT(MethodGenerationContext& mgenc, long idx, int ctx) {
    assert(idx >= 0);
    assert(ctx >= 0);

    if (ctx == 0) {
        if (idx == 0) {
            Emit1(mgenc, BC_PUSH_SELF, 1);
            return;
        }

        if (idx == 1) {
            Emit1(mgenc, BC_PUSH_ARG_1, 1);
            return;
        }

        if (idx == 2) {
            Emit1(mgenc, BC_PUSH_ARG_2, 1);
            return;
        }
    }
    Emit3(mgenc, BC_PUSH_ARGUMENT, idx, ctx, 1);
}

void EmitPUSHFIELD(MethodGenerationContext& mgenc, VMSymbol* field) {
    const uint8_t idx = mgenc.GetFieldIndex(field);
    if (idx == 0) {
        Emit1(mgenc, BC_PUSH_FIELD_0, 1);
    } else if (idx == 1) {
        Emit1(mgenc, BC_PUSH_FIELD_1, 1);
    } else {
        Emit2(mgenc, BC_PUSH_FIELD, idx, 1);
    }
}

void EmitPUSHBLOCK(MethodGenerationContext& mgenc, VMMethod* block) {
    const int8_t idx = mgenc.AddLiteralIfAbsent(block);
    Emit2(mgenc, BC_PUSH_BLOCK, idx, 1);
}

void EmitPUSHCONSTANT(MethodGenerationContext& mgenc, vm_oop_t cst) {
    // this isn't very robust with respect to initialization order
    // so, we check here, and hope it's working, but alternatively
    // we also make sure that we don't miss anything in the else
    // branch of the class check
    if (CLASS_OF(cst) == load_ptr(integerClass)) {
        if (INT_VAL(cst) == 0ll) {
            Emit1(mgenc, BC_PUSH_0, 1);
            return;
        }
        if (INT_VAL(cst) == 1ll) {
            Emit1(mgenc, BC_PUSH_1, 1);
            return;
        }
    } else {
        assert(!IsVMInteger(cst));
    }

    if (cst == load_ptr(nilObject)) {
        Emit1(mgenc, BC_PUSH_NIL, 1);
        return;
    }

    const int8_t idx = mgenc.AddLiteralIfAbsent(cst);
    if (idx == 0) {
        Emit1(mgenc, BC_PUSH_CONSTANT_0, 1);
        return;
    }
    if (idx == 1) {
        Emit1(mgenc, BC_PUSH_CONSTANT_1, 1);
        return;
    }
    if (idx == 2) {
        Emit1(mgenc, BC_PUSH_CONSTANT_2, 1);
        return;
    }

    Emit2(mgenc, BC_PUSH_CONSTANT, idx, 1);
}

void EmitPUSHCONSTANT(MethodGenerationContext& mgenc, uint8_t literalIndex) {
    Emit2(mgenc, BC_PUSH_CONSTANT, literalIndex, 1);
}

void EmitPUSHCONSTANTString(MethodGenerationContext& mgenc, VMString* str) {
    Emit2(mgenc, BC_PUSH_CONSTANT, mgenc.FindLiteralIndex(str), 1);
}

void EmitPUSHGLOBAL(MethodGenerationContext& mgenc, VMSymbol* global) {
    if (global == SymbolFor("nil")) {
        EmitPUSHCONSTANT(mgenc, load_ptr(nilObject));
    } else if (global == SymbolFor("true")) {
        EmitPUSHCONSTANT(mgenc, load_ptr(trueObject));
    } else if (global == SymbolFor("false")) {
        EmitPUSHCONSTANT(mgenc, load_ptr(falseObject));
    } else {
        const int8_t idx = mgenc.AddLiteralIfAbsent(global);
        Emit2(mgenc, BC_PUSH_GLOBAL, idx, 1);
    }
}

void EmitPOP(MethodGenerationContext& mgenc) {
    Emit1(mgenc, BC_POP, -1);
}

void EmitPOPLOCAL(MethodGenerationContext& mgenc, long idx, int ctx) {
    assert(idx >= 0);
    assert(ctx >= 0);
    if (ctx == 0) {
        if (idx == 0) {
            Emit1(mgenc, BC_POP_LOCAL_0, -1);
            return;
        }

        if (idx == 1) {
            Emit1(mgenc, BC_POP_LOCAL_1, -1);
            return;
        }

        if (idx == 2) {
            Emit1(mgenc, BC_POP_LOCAL_2, -1);
            return;
        }
    }

    Emit3(mgenc, BC_POP_LOCAL, idx, ctx, -1);
}

void EmitPOPARGUMENT(MethodGenerationContext& mgenc, long idx, int ctx) {
    Emit3(mgenc, BC_POP_ARGUMENT, idx, ctx, -1);
}

void EmitPOPFIELD(MethodGenerationContext& mgenc, VMSymbol* field) {
    const uint8_t idx = mgenc.GetFieldIndex(field);

    if (idx == 0) {
        Emit1(mgenc, BC_POP_FIELD_0, -1);
    } else if (idx == 1) {
        Emit1(mgenc, BC_POP_FIELD_1, -1);
    } else {
        Emit2(mgenc, BC_POP_FIELD, idx, -1);
    }
}

void EmitSEND(MethodGenerationContext& mgenc, VMSymbol* msg) {
    const int8_t idx = mgenc.AddLiteralIfAbsent(msg);

    const int numArgs = Signature::GetNumberOfArguments(msg);
    const size_t stackEffect = -numArgs + 1;  // +1 for the result

    Emit2(mgenc, BC_SEND, idx, stackEffect);
}

void EmitSUPERSEND(MethodGenerationContext& mgenc, VMSymbol* msg) {
    const int8_t idx = mgenc.AddLiteralIfAbsent(msg);

    const int numArgs = Signature::GetNumberOfArguments(msg);
    const size_t stackEffect = -numArgs + 1;  // +1 for the result

    Emit2(mgenc, BC_SUPER_SEND, idx, stackEffect);
}

void EmitRETURNSELF(MethodGenerationContext& mgenc) {
    Emit1(mgenc, BC_RETURN_SELF, 0);
}

void EmitRETURNLOCAL(MethodGenerationContext& mgenc) {
    Emit1(mgenc, BC_RETURN_LOCAL, 0);
}

void EmitRETURNNONLOCAL(MethodGenerationContext& mgenc) {
    Emit1(mgenc, BC_RETURN_NON_LOCAL, 0);
}

void EmitINC(MethodGenerationContext& mgenc) {
    Emit1(mgenc, BC_INC, 0);
}

void EmitDupSecond(MethodGenerationContext& mgenc) {
    Emit1(mgenc, BC_DUP_SECOND, 1);
}

size_t EmitJumpOnBoolWithDummyOffset(MethodGenerationContext& mgenc,
                                     bool isIfTrue, bool needsPop) {
    // Remember: true and false seem flipped here.
    // This is because if the test passes, the block is inlined directly.
    // But if the test fails, we need to jump.
    // Thus, an  `#ifTrue:` needs to generated a jump_on_false.
    uint8_t bc;
    size_t stackEffect;

    if (needsPop) {
        bc = isIfTrue ? BC_JUMP_ON_FALSE_POP : BC_JUMP_ON_TRUE_POP;
        stackEffect = -1;
    } else {
        bc = isIfTrue ? BC_JUMP_ON_FALSE_TOP_NIL : BC_JUMP_ON_TRUE_TOP_NIL;
        stackEffect = 0;
    }

    Emit1(mgenc, bc, stackEffect);

    size_t idx = mgenc.AddBytecodeArgumentAndGetIndex(0);
    mgenc.AddBytecodeArgument(0);
    return idx;
}

size_t EmitJumpWithDumyOffset(MethodGenerationContext& mgenc) {
    Emit1(mgenc, BC_JUMP, 0);
    size_t idx = mgenc.AddBytecodeArgumentAndGetIndex(0);
    mgenc.AddBytecodeArgument(0);
    return idx;
}

size_t EmitJumpIfGreaterWithDummyOffset(MethodGenerationContext& mgenc) {
    Emit1(mgenc, BC_JUMP_IF_GREATER, 0);
    size_t idx = mgenc.AddBytecodeArgumentAndGetIndex(0);
    mgenc.AddBytecodeArgument(0);
    return idx;
}

void EmitJumpBackwardWithOffset(MethodGenerationContext& mgenc,
                                size_t jumpOffset) {
    uint8_t jumpBytecode =
        jumpOffset <= 0xFF ? BC_JUMP_BACKWARD : BC_JUMP2_BACKWARD;
    Emit3(mgenc, jumpBytecode, jumpOffset & 0xFF, jumpOffset >> 8, 0);
}

size_t Emit3WithDummy(MethodGenerationContext& mgenc, uint8_t bytecode,
                      size_t stackEffect) {
    mgenc.AddBytecode(bytecode, stackEffect);
    size_t index = mgenc.AddBytecodeArgumentAndGetIndex(0);
    mgenc.AddBytecodeArgument(0);
    return index;
}

void EmitPushFieldWithIndex(MethodGenerationContext& mgenc, uint8_t fieldIdx,
                            uint8_t ctxLevel) {
    // if (ctxLevel == 0) {
    if (fieldIdx == 0) {
        Emit1(mgenc, BC_PUSH_FIELD_0, 1);
        return;
    }

    if (fieldIdx == 1) {
        Emit1(mgenc, BC_PUSH_FIELD_1, 1);
        return;
    }
    // }

    Emit2(mgenc, BC_PUSH_FIELD, fieldIdx, 1);
}

void EmitPopFieldWithIndex(MethodGenerationContext& mgenc, uint8_t fieldIdx,
                           uint8_t ctxLevel) {
    // if (ctxLevel == 0) {
    if (fieldIdx == 0) {
        Emit1(mgenc, BC_POP_FIELD_0, 1);
        return;
    }

    if (fieldIdx == 1) {
        Emit1(mgenc, BC_POP_FIELD_1, 1);
        return;
    }
    // }

    Emit2(mgenc, BC_POP_FIELD, fieldIdx, 1);
}
