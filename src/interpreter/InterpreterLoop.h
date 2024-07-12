#ifndef HACK_INLINE_START
vm_oop_t Start() {
#endif
    // initialization
    method = GetMethod();
    currentBytecodes = GetBytecodes();
    
    void* loopTargets[] = {
        &&LABEL_BC_HALT,
        &&LABEL_BC_DUP,
        &&LABEL_BC_PUSH_LOCAL,
        &&LABEL_BC_PUSH_LOCAL_0,
        &&LABEL_BC_PUSH_LOCAL_1,
        &&LABEL_BC_PUSH_LOCAL_2,
        &&LABEL_BC_PUSH_ARGUMENT,
        &&LABEL_BC_PUSH_SELF,
        &&LABEL_BC_PUSH_ARG_1,
        &&LABEL_BC_PUSH_ARG_2,
        &&LABEL_BC_PUSH_FIELD,
        &&LABEL_BC_PUSH_FIELD_0,
        &&LABEL_BC_PUSH_FIELD_1,
        &&LABEL_BC_PUSH_BLOCK,
        &&LABEL_BC_PUSH_CONSTANT,
        &&LABEL_BC_PUSH_CONSTANT_0,
        &&LABEL_BC_PUSH_CONSTANT_1,
        &&LABEL_BC_PUSH_CONSTANT_2,
        &&LABEL_BC_PUSH_0,
        &&LABEL_BC_PUSH_1,
        &&LABEL_BC_PUSH_NIL,
        &&LABEL_BC_PUSH_GLOBAL,
        &&LABEL_BC_POP,
        &&LABEL_BC_POP_LOCAL,
        &&LABEL_BC_POP_LOCAL_0,
        &&LABEL_BC_POP_LOCAL_1,
        &&LABEL_BC_POP_LOCAL_2,
        &&LABEL_BC_POP_ARGUMENT,
        &&LABEL_BC_POP_FIELD,
        &&LABEL_BC_POP_FIELD_0,
        &&LABEL_BC_POP_FIELD_1,
        &&LABEL_BC_SEND,
        &&LABEL_BC_SUPER_SEND,
        &&LABEL_BC_RETURN_LOCAL,
        &&LABEL_BC_RETURN_NON_LOCAL,
        &&LABEL_BC_JUMP_IF_FALSE,
        &&LABEL_BC_JUMP_IF_TRUE,
        &&LABEL_BC_JUMP
    };
    
    goto *loopTargets[currentBytecodes[bytecodeIndexGlobal]];
    
    //
    // THIS IS THE former interpretation loop
LABEL_BC_HALT:
    PROLOGUE(1);
    return GetFrame()->GetStackElement(0); // handle the halt bytecode
    
LABEL_BC_DUP:
    PROLOGUE(1);
    doDup();
    DISPATCH_NOGC();
    
LABEL_BC_PUSH_LOCAL:
    PROLOGUE(3);
    doPushLocal(bytecodeIndexGlobal - 3);
    DISPATCH_NOGC();

LABEL_BC_PUSH_LOCAL_0:
    PROLOGUE(1);
    doPushLocalWithIndex(0);
    DISPATCH_NOGC();
    
LABEL_BC_PUSH_LOCAL_1:
    PROLOGUE(1);
    doPushLocalWithIndex(1);
    DISPATCH_NOGC();
    
LABEL_BC_PUSH_LOCAL_2:
    PROLOGUE(1);
    doPushLocalWithIndex(2);
    DISPATCH_NOGC();
    
LABEL_BC_PUSH_ARGUMENT:
LABEL_BC_PUSH_SELF:
LABEL_BC_PUSH_ARG_1:
LABEL_BC_PUSH_ARG_2:
    PROLOGUE(3);
    doPushArgument(bytecodeIndexGlobal - 3);
    DISPATCH_NOGC();
    
LABEL_BC_PUSH_FIELD:
LABEL_BC_PUSH_FIELD_0:
LABEL_BC_PUSH_FIELD_1:
    PROLOGUE(2);
    doPushField(bytecodeIndexGlobal - 2);
    DISPATCH_NOGC();
    
LABEL_BC_PUSH_BLOCK:
    PROLOGUE(2);
    doPushBlock(bytecodeIndexGlobal - 2);
    DISPATCH_GC();
    
LABEL_BC_PUSH_CONSTANT:
    PROLOGUE(2);
    doPushConstant(bytecodeIndexGlobal - 2);
    DISPATCH_NOGC();
    
LABEL_BC_PUSH_CONSTANT_0:
    PROLOGUE(1);
    {
        vm_oop_t constant = method->GetIndexableField(0);
        GetFrame()->Push(constant);
    }
    DISPATCH_NOGC();
    
LABEL_BC_PUSH_CONSTANT_1:
    PROLOGUE(1);
    {
        vm_oop_t constant = method->GetIndexableField(1);
        GetFrame()->Push(constant);
    }
    DISPATCH_NOGC();
    
LABEL_BC_PUSH_CONSTANT_2:
    PROLOGUE(1);
    {
        vm_oop_t constant = method->GetIndexableField(2);
        GetFrame()->Push(constant);
    }
    DISPATCH_NOGC();

LABEL_BC_PUSH_0:
    PROLOGUE(1);
    GetFrame()->Push(NEW_INT(0));
    DISPATCH_NOGC();
    
LABEL_BC_PUSH_1:
    PROLOGUE(1);
    GetFrame()->Push(NEW_INT(1));
    DISPATCH_NOGC();
    
LABEL_BC_PUSH_NIL:
    PROLOGUE(1);
    GetFrame()->Push(load_ptr(nilObject));
    DISPATCH_NOGC();
    
LABEL_BC_PUSH_GLOBAL:
    PROLOGUE(2);
    doPushGlobal(bytecodeIndexGlobal - 2);
    DISPATCH_GC();
    
LABEL_BC_POP:
    PROLOGUE(1);
    doPop();
    DISPATCH_NOGC();
    
LABEL_BC_POP_LOCAL:
LABEL_BC_POP_LOCAL_0:
LABEL_BC_POP_LOCAL_1:
LABEL_BC_POP_LOCAL_2:
    PROLOGUE(3);
    doPopLocal(bytecodeIndexGlobal - 3);
    DISPATCH_NOGC();
    
LABEL_BC_POP_ARGUMENT:
    PROLOGUE(3);
    doPopArgument(bytecodeIndexGlobal - 3);
    DISPATCH_NOGC();
    
LABEL_BC_POP_FIELD:
LABEL_BC_POP_FIELD_0:
LABEL_BC_POP_FIELD_1:
    PROLOGUE(2);
    doPopField(bytecodeIndexGlobal - 2);
    DISPATCH_NOGC();
    
LABEL_BC_SEND:
    PROLOGUE(2);
    doSend(bytecodeIndexGlobal - 2);
    DISPATCH_GC();
    
LABEL_BC_SUPER_SEND:
    PROLOGUE(2);
    doSuperSend(bytecodeIndexGlobal - 2);
    DISPATCH_GC();
    
LABEL_BC_RETURN_LOCAL:
    PROLOGUE(1);
    doReturnLocal();
    DISPATCH_NOGC();
    
LABEL_BC_RETURN_NON_LOCAL:
    PROLOGUE(1);
    doReturnNonLocal();
    DISPATCH_NOGC();
    
LABEL_BC_JUMP_IF_FALSE:
    PROLOGUE(5);
    doJumpIfFalse(bytecodeIndexGlobal - 5);
    DISPATCH_NOGC();
    
LABEL_BC_JUMP_IF_TRUE:
    PROLOGUE(5);
    doJumpIfTrue(bytecodeIndexGlobal - 5);
    DISPATCH_NOGC();
    
LABEL_BC_JUMP:
    PROLOGUE(5);
    doJump(bytecodeIndexGlobal - 5);
    DISPATCH_NOGC();

#ifndef HACK_INLINE_START
}
#endif
