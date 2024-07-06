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

#include <misc/defs.h>
#include <vmobjects/ObjectFormats.h>
#include <vmobjects/VMFrame.h>

#define PROLOGUE(bc_count) {\
  if (printBytecodes) disassembleMethod(); \
    bytecodeIndexGlobal += bc_count;\
  }

#define DISPATCH_NOGC() {\
  goto *loopTargets[currentBytecodes[bytecodeIndexGlobal]]; \
}

#define DISPATCH_GC() {\
  triggerGC(); \
  goto *loopTargets[currentBytecodes[bytecodeIndexGlobal]];\
}


class Interpreter {
public:
    Interpreter();
    ~Interpreter();
    
    template<bool printBytecodes>
    vm_oop_t  Start() {
        // initialization
        method = GetMethod();
        currentBytecodes = GetBytecodes();

        void* loopTargets[] = {
            &&LABEL_BC_HALT,
            &&LABEL_BC_DUP,
            &&LABEL_BC_PUSH_LOCAL,
            &&LABEL_BC_PUSH_ARGUMENT,
            &&LABEL_BC_PUSH_FIELD,
            &&LABEL_BC_PUSH_BLOCK,
            &&LABEL_BC_PUSH_CONSTANT,
            &&LABEL_BC_PUSH_GLOBAL,
            &&LABEL_BC_POP,
            &&LABEL_BC_POP_LOCAL,
            &&LABEL_BC_POP_ARGUMENT,
            &&LABEL_BC_POP_FIELD,
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

        LABEL_BC_PUSH_ARGUMENT:
          PROLOGUE(3);
          doPushArgument(bytecodeIndexGlobal - 3);
          DISPATCH_NOGC();

        LABEL_BC_PUSH_FIELD:
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

        LABEL_BC_POP_ARGUMENT:
          PROLOGUE(3);
          doPopArgument(bytecodeIndexGlobal - 3);
          DISPATCH_NOGC();

        LABEL_BC_POP_FIELD:
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
    }
    
    VMFrame*  PushNewFrame(VMMethod* method);
    void      SetFrame(VMFrame* frame);
    inline VMFrame* GetFrame() const;
    inline VMMethod* GetMethod() const;
    inline uint8_t* GetBytecodes() const;
    void      WalkGlobals(walk_heap_fn);
    
private:
    vm_oop_t GetSelf() const;
    
    VMFrame* frame;
    VMMethod* method;
    
    // The following three variables are used to cache main parts of the
    // current execution context
    long      bytecodeIndexGlobal;
    uint8_t*  currentBytecodes;

    
    static const StdString unknownGlobal;
    static const StdString doesNotUnderstand;
    static const StdString escapedBlock;
    
    void triggerGC();
    void disassembleMethod() const;

    VMFrame* popFrame();
    void popFrameAndPushResult(vm_oop_t result);
    void send(VMSymbol* signature, VMClass* receiverClass);

    void doDup();
    void doPushLocal(long bytecodeIndex);
    void doPushArgument(long bytecodeIndex);
    void doPushField(long bytecodeIndex);
    void doPushBlock(long bytecodeIndex);
    void doPushConstant(long bytecodeIndex);
    void doPushGlobal(long bytecodeIndex);
    void doPop(void);
    void doPopLocal(long bytecodeIndex);
    void doPopArgument(long bytecodeIndex);
    void doPopField(long bytecodeIndex);
    void doSend(long bytecodeIndex);
    void doSuperSend(long bytecodeIndex);
    void doReturnLocal();
    void doReturnNonLocal();
    void doJumpIfFalse(long bytecodeIndex);
    void doJumpIfTrue(long bytecodeIndex);
    void doJump(long bytecodeIndex);
};

inline VMFrame* Interpreter::GetFrame() const {
    return frame;
}
