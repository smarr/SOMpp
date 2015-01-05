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

class Interpreter {
public:
    Interpreter();
    ~Interpreter();
    
    void      Start();
    VMFrame*  PushNewFrame(VMMethod* method);
    void      SetFrame(VMFrame* frame);
    inline VMFrame* GetFrame() const;
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

VMFrame* Interpreter::GetFrame() const {
    return frame;
}
