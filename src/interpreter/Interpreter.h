#pragma once
#ifndef INTERPRETER_H_
#define INTERPRETER_H_

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

#include "../misc/defs.h"

#include "../vmobjects/ObjectFormats.h"
#include "../memory/Page.h"

#if GC_TYPE==PAUSELESS
#include "../memory/pauseless/Worklist.h"
#endif

class VMMethod;
class VMFrame;
class VMMethod;
class VMObject;
class VMSymbol;
class VMClass;
class AbstractVMObject;
class VMThread;

class Interpreter {
public:
    Interpreter();
    ~Interpreter();
    
    void      Start();
    pVMThread GetThread();
    void      SetThread(pVMThread thread);
    pVMFrame  PushNewFrame(pVMMethod method);
    void      SetFrame(pVMFrame frame);
    pVMFrame  GetFrame();
    pVMObject GetSelf();
    Page*     GetPage();
    void      SetPage(Page*);
    
#if GC_TYPE==PAUSELESS
    void      MoveWork(Worklist*);
    void      AddGCWork(VMOBJECT_PTR);
    void      EnableBlocked();
    void      DisableBlocked();
    void      TriggerMarkRootSet();
    void      MarkRootSet();
    void      DisableGCTrap();
    void      SignalEnableGCTrap();
    void      EnableGCTrap();
    bool      GCTrapEnabled();
    bool      GetExpectedNMT();
#else
    void      WalkGlobals(VMOBJECT_PTR (*walk)(VMOBJECT_PTR));
#endif
    
private:
    pVMThread thread;
    pVMFrame frame;
    StdString uG;
    StdString dnu;
    StdString eB;

    Page* page;
    
    pVMFrame popFrame();
    void popFrameAndPushResult(pVMObject result);
    void send(pVMSymbol signature, pVMClass receiverClass);
    
    pVMMethod GetMethod();

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
    
    // The following three variables are used to cache main parts of the
    // current execution context
    long      bytecodeIndexGlobal;
    pVMMethod method;
    uint8_t*  currentBytecodes;
    
#if GC_TYPE==PAUSELESS
    Worklist worklist;
    bool blocked;
    bool markRootSet;
    bool expectedNMT;
    bool gcTrapEnabled;
    bool signalEnableGCTrap;
    bool trapTriggered; //this variable is used to identy mutators having triggered NMT-traps and which not
    
    pthread_mutex_t blockedMutex;
    
#endif
    
    
};

#endif
