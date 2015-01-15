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
#include "../memory/BaseThread.h"

class VMMethod;
class VMFrame;
class VMMethod;
class VMObject;
class VMSymbol;
class VMClass;
class AbstractVMObject;
class VMThread;

class Page;

class Interpreter : public BaseThread {
public:
    
#if GC_TYPE!=PAUSELESS
    Interpreter();
#else
    Interpreter(bool, bool);
#endif
    //~Interpreter();
    
    void      Start();
    pVMThread GetThread();
    void      SetThread(pVMThread thread);
    pVMFrame  PushNewFrame(pVMMethod method);
    void      SetFrame(pVMFrame frame);
    pVMFrame  GetFrame();
    pVMObject GetSelf();
    
#if GC_TYPE==PAUSELESS
    virtual void AddGCWork(AbstractVMObject*);
    void         EnableBlocked();
    void         DisableBlocked();
    void         TriggerMarkRootSet();
    void         MarkRootSet();
    void         MarkRootSetByGC();
    void         DummyMarkRootSet();
    void         ResetAlreadyMarked();
    void         RequestSafePoint();
    void         CancelSafePoint();
    void         SignalSafepointReached();
    void         DisableGCTrap();
    void         SignalEnableGCTrap();
    void         EnableGCTrap();
    bool         GCTrapEnabled();
    FORCE_INLINE bool         GetExpectedNMT() { return expectedNMT; }
    void         AddFullPage(Page*);
    Page*        GetNonRelocatablePage();
    void         SetNonRelocatablePage(Page*);
    void         AddFullNonRelocatablePage(Page*);
    void         EnableStop();
    
    bool         TriggerGCTrap(Page*);
    
    // for debugging purposes
    void         CheckMarking(void (AbstractVMObject*));
#else
    void      WalkGlobals(VMOBJECT_PTR (*walk)(VMOBJECT_PTR));
#endif
    
private:
    GCThread* thread;
    GCFrame* frame;
    StdString uG;
    StdString dnu;
    StdString eB;
    
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

    //    pVMMethod method; // not safe for parallel moving
    //    uint8_t*  currentBytecodes;
    
#if GC_TYPE==PAUSELESS
    bool stopped;
    bool blocked;
    
    bool markRootSet;
    bool safePointRequested;
    bool signalEnableGCTrap;
    bool gcTrapEnabled;
    
    Page* nonRelocatablePage;
    vector<Page*> fullPages;
    vector<Page*> nonRelocatablePages;

    pthread_mutex_t blockedMutex;
    
    pthread_mutex_t prevent;
#endif
    
    
};

#endif
