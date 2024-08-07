#ifndef HACK_INLINE_START
vm_oop_t Start() {
#endif
    // initialization
    method = GetMethod();
    currentBytecodes = GetBytecodes();

    void* loopTargets[] = {&&LABEL_BC_HALT,
                           &&LABEL_BC_DUP,
                           &&LABEL_BC_DUP_SECOND,
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
                           &&LABEL_BC_RETURN_SELF,
                           &&LABEL_BC_RETURN_FIELD_0,
                           &&LABEL_BC_RETURN_FIELD_1,
                           &&LABEL_BC_RETURN_FIELD_2,
                           &&LABEL_BC_INC,
                           &&LABEL_BC_JUMP,
                           &&LABEL_BC_JUMP_ON_FALSE_POP,
                           &&LABEL_BC_JUMP_ON_TRUE_POP,
                           &&LABEL_BC_JUMP_ON_FALSE_TOP_NIL,
                           &&LABEL_BC_JUMP_ON_TRUE_TOP_NIL,
                           &&LABEL_BC_JUMP_IF_GREATER,
                           &&LABEL_BC_JUMP_BACKWARD,
                           &&LABEL_BC_JUMP2,
                           &&LABEL_BC_JUMP2_ON_FALSE_POP,
                           &&LABEL_BC_JUMP2_ON_TRUE_POP,
                           &&LABEL_BC_JUMP2_ON_FALSE_TOP_NIL,
                           &&LABEL_BC_JUMP2_ON_TRUE_TOP_NIL,
                           &&LABEL_BC_JUMP2_IF_GREATER,
                           &&LABEL_BC_JUMP2_BACKWARD};

    goto* loopTargets[currentBytecodes[bytecodeIndexGlobal]];

    //
    // THIS IS THE former interpretation loop
LABEL_BC_HALT:
    PROLOGUE(1);
    return GetFrame()->GetStackElement(0);  // handle the halt bytecode

LABEL_BC_DUP:
    PROLOGUE(1);
    doDup();
    DISPATCH_NOGC();

LABEL_BC_DUP_SECOND:
    PROLOGUE(1);
    {
        vm_oop_t elem = GetFrame()->GetStackElement(1);
        GetFrame()->Push(elem);
    }
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
    PROLOGUE(3);
    doPushArgument(bytecodeIndexGlobal - 3);
    DISPATCH_NOGC();

LABEL_BC_PUSH_SELF:
    PROLOGUE(1);
    {
        vm_oop_t argument = GetFrame()->GetArgumentInCurrentContext(0);
        GetFrame()->Push(argument);
    }
    DISPATCH_NOGC();

LABEL_BC_PUSH_ARG_1:
    PROLOGUE(1);
    {
        vm_oop_t argument = GetFrame()->GetArgumentInCurrentContext(1);
        GetFrame()->Push(argument);
    }
    DISPATCH_NOGC();

LABEL_BC_PUSH_ARG_2:
    PROLOGUE(1);
    {
        vm_oop_t argument = GetFrame()->GetArgumentInCurrentContext(2);
        GetFrame()->Push(argument);
    }
    DISPATCH_NOGC();

LABEL_BC_PUSH_FIELD:
    PROLOGUE(2);
    doPushField(bytecodeIndexGlobal - 2);
    DISPATCH_NOGC();

LABEL_BC_PUSH_FIELD_0:
    PROLOGUE(1);
    doPushFieldWithIndex(0);
    DISPATCH_NOGC();

LABEL_BC_PUSH_FIELD_1:
    PROLOGUE(1);
    doPushFieldWithIndex(1);
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
    PROLOGUE(3);
    doPopLocal(bytecodeIndexGlobal - 3);
    DISPATCH_NOGC();

LABEL_BC_POP_LOCAL_0:
    PROLOGUE(1);
    doPopLocalWithIndex(0);
    DISPATCH_NOGC();

LABEL_BC_POP_LOCAL_1:
    PROLOGUE(1);
    doPopLocalWithIndex(1);
    DISPATCH_NOGC();

LABEL_BC_POP_LOCAL_2:
    PROLOGUE(1);
    doPopLocalWithIndex(2);
    DISPATCH_NOGC();

LABEL_BC_POP_ARGUMENT:
    PROLOGUE(3);
    doPopArgument(bytecodeIndexGlobal - 3);
    DISPATCH_NOGC();

LABEL_BC_POP_FIELD:
    PROLOGUE(2);
    doPopField(bytecodeIndexGlobal - 2);
    DISPATCH_NOGC();

LABEL_BC_POP_FIELD_0:
    PROLOGUE(1);
    doPopFieldWithIndex(0);
    DISPATCH_NOGC();

LABEL_BC_POP_FIELD_1:
    PROLOGUE(1);
    doPopFieldWithIndex(1);
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

LABEL_BC_RETURN_SELF: {
    PROLOGUE(1);
    assert(GetFrame()->GetContext() == nullptr &&
           "RETURN_SELF is not allowed in blocks");
    popFrameAndPushResult(GetFrame()->GetArgumentInCurrentContext(0));
    DISPATCH_NOGC();
}

LABEL_BC_RETURN_FIELD_0:
    PROLOGUE(1);
    doReturnFieldWithIndex(0);
    DISPATCH_NOGC();

LABEL_BC_RETURN_FIELD_1:
    PROLOGUE(1);
    doReturnFieldWithIndex(1);
    DISPATCH_NOGC();

LABEL_BC_RETURN_FIELD_2:
    PROLOGUE(1);
    doReturnFieldWithIndex(2);
    DISPATCH_NOGC();

LABEL_BC_INC:
    PROLOGUE(1);
    doInc();
    DISPATCH_NOGC();

LABEL_BC_JUMP: {
    uint8_t offset = currentBytecodes[bytecodeIndexGlobal + 1];
    bytecodeIndexGlobal += offset;
}
    DISPATCH_NOGC();

LABEL_BC_JUMP_ON_FALSE_POP: {
    vm_oop_t val = GetFrame()->Top();
    if (val == load_ptr(falseObject)) {
        uint8_t offset = currentBytecodes[bytecodeIndexGlobal + 1];
        bytecodeIndexGlobal += offset;
    } else {
        bytecodeIndexGlobal += 3;
    }
    GetFrame()->PopVoid();
}
    DISPATCH_NOGC();

LABEL_BC_JUMP_ON_TRUE_POP: {
    vm_oop_t val = GetFrame()->Top();
    if (val == load_ptr(trueObject)) {
        uint8_t offset = currentBytecodes[bytecodeIndexGlobal + 1];
        bytecodeIndexGlobal += offset;
    } else {
        bytecodeIndexGlobal += 3;
    }
    GetFrame()->PopVoid();
}
    DISPATCH_NOGC();

LABEL_BC_JUMP_ON_FALSE_TOP_NIL: {
    vm_oop_t val = GetFrame()->Top();
    if (val == load_ptr(falseObject)) {
        uint8_t offset = currentBytecodes[bytecodeIndexGlobal + 1];
        bytecodeIndexGlobal += offset;
        GetFrame()->SetTop(nilObject);
    } else {
        GetFrame()->PopVoid();
        bytecodeIndexGlobal += 3;
    }
}
    DISPATCH_NOGC();

LABEL_BC_JUMP_ON_TRUE_TOP_NIL: {
    vm_oop_t val = GetFrame()->Top();
    if (val == load_ptr(trueObject)) {
        uint8_t offset = currentBytecodes[bytecodeIndexGlobal + 1];
        bytecodeIndexGlobal += offset;
        GetFrame()->SetTop(nilObject);
    } else {
        GetFrame()->PopVoid();
        bytecodeIndexGlobal += 3;
    }
}
    DISPATCH_NOGC();

LABEL_BC_JUMP_IF_GREATER: {
    if (checkIsGreater()) {
        bytecodeIndexGlobal += currentBytecodes[bytecodeIndexGlobal + 1];
        GetFrame()->Pop();
        GetFrame()->Pop();
    } else {
        bytecodeIndexGlobal += 3;
    }
}
    DISPATCH_NOGC();

LABEL_BC_JUMP_BACKWARD: {
    uint8_t offset = currentBytecodes[bytecodeIndexGlobal + 1];
    bytecodeIndexGlobal -= offset;
}
    DISPATCH_NOGC();

LABEL_BC_JUMP2: {
    uint16_t offset = ComputeOffset(currentBytecodes[bytecodeIndexGlobal + 1],
                                    currentBytecodes[bytecodeIndexGlobal + 2]);
    bytecodeIndexGlobal += offset;
}
    DISPATCH_NOGC();

LABEL_BC_JUMP2_ON_FALSE_POP: {
    vm_oop_t val = GetFrame()->Top();
    if (val == load_ptr(falseObject)) {
        uint16_t offset =
            ComputeOffset(currentBytecodes[bytecodeIndexGlobal + 1],
                          currentBytecodes[bytecodeIndexGlobal + 2]);
        bytecodeIndexGlobal += offset;
    } else {
        bytecodeIndexGlobal += 3;
    }
    GetFrame()->PopVoid();
}
    DISPATCH_NOGC();

LABEL_BC_JUMP2_ON_TRUE_POP: {
    vm_oop_t val = GetFrame()->Top();
    if (val == load_ptr(trueObject)) {
        uint16_t offset =
            ComputeOffset(currentBytecodes[bytecodeIndexGlobal + 1],
                          currentBytecodes[bytecodeIndexGlobal + 2]);
        bytecodeIndexGlobal += offset;
    } else {
        bytecodeIndexGlobal += 3;
    }
    GetFrame()->PopVoid();
}
    DISPATCH_NOGC();

LABEL_BC_JUMP2_ON_FALSE_TOP_NIL: {
    vm_oop_t val = GetFrame()->Top();
    if (val == load_ptr(falseObject)) {
        uint16_t offset =
            ComputeOffset(currentBytecodes[bytecodeIndexGlobal + 1],
                          currentBytecodes[bytecodeIndexGlobal + 2]);
        bytecodeIndexGlobal += offset;
        GetFrame()->SetTop(nilObject);
    } else {
        GetFrame()->PopVoid();
        bytecodeIndexGlobal += 3;
    }
}
    DISPATCH_NOGC();

LABEL_BC_JUMP2_ON_TRUE_TOP_NIL: {
    vm_oop_t val = GetFrame()->Top();
    if (val == load_ptr(trueObject)) {
        uint16_t offset =
            ComputeOffset(currentBytecodes[bytecodeIndexGlobal + 1],
                          currentBytecodes[bytecodeIndexGlobal + 2]);
        bytecodeIndexGlobal += offset;
        GetFrame()->SetTop(nilObject);
    } else {
        GetFrame()->PopVoid();
        bytecodeIndexGlobal += 3;
    }
}
    DISPATCH_NOGC();

LABEL_BC_JUMP2_IF_GREATER: {
    if (checkIsGreater()) {
        bytecodeIndexGlobal +=
            ComputeOffset(currentBytecodes[bytecodeIndexGlobal + 1],
                          currentBytecodes[bytecodeIndexGlobal + 2]);
        GetFrame()->Pop();
        GetFrame()->Pop();
    } else {
        bytecodeIndexGlobal += 3;
    }
}
    DISPATCH_NOGC();

LABEL_BC_JUMP2_BACKWARD: {
    uint16_t offset = ComputeOffset(currentBytecodes[bytecodeIndexGlobal + 1],
                                    currentBytecodes[bytecodeIndexGlobal + 2]);
    bytecodeIndexGlobal -= offset;
}
    DISPATCH_NOGC();

#ifndef HACK_INLINE_START
}
#endif
