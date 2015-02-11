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

#include <misc/debug.h>
#include "Interpreter.h"
#include "Interpreter.inline.h"
#include "bytecodes.h"

#include <vmobjects/VMMethod.h>
#include <vmobjects/VMFrame.h>
#include <vmobjects/VMMethod.h>
#include <vmobjects/VMClass.h>
#include <vmobjects/VMObject.h>
#include <vmobjects/VMSymbol.h>
#include <vmobjects/VMArray.h>
#include <vmobjects/VMInvokable.h>
#include <vmobjects/Signature.h>
#include <vmobjects/VMBlock.h>
#include <vmobjects/IntegerBox.h>

#include <compiler/Disassembler.h>
#include <vmobjects/VMBlock.inline.h>
#include <vmobjects/VMMethod.inline.h>

const StdString Interpreter::unknownGlobal     = "unknownGlobal:";
const StdString Interpreter::doesNotUnderstand = "doesNotUnderstand:arguments:";
const StdString Interpreter::escapedBlock      = "escapedBlock:";


#if GC_TYPE==PAUSELESS
Interpreter::Interpreter(Page* page, bool expectedNMT, bool gcTrapEnabled) : BaseThread(page, expectedNMT) {
    frame = nullptr;

    stopped = false;
    blocked = false;
    markRootSet = false;
    safePointRequested = false;
    signalEnableGCTrap = false;
    this->gcTrapEnabled = gcTrapEnabled;
    pthread_mutex_init(&blockedMutex, nullptr);
}
#else
Interpreter::Interpreter(Page* page) : BaseThread(page) {
    frame = nullptr;
}
#endif

#define PROLOGUE(bc_count) {\
  if (dumpBytecodes > 1) Disassembler::DumpBytecode(GetFrame(), GetFrame()->GetMethod(), bytecodeIndexGlobal);\
  bytecodeIndexGlobal += bc_count;\
}

#define DISPATCH_NOGC() {\
  goto *loopTargets[currentBytecodes[bytecodeIndexGlobal]]; \
}

#if GC_TYPE==PAUSELESS
#define DISPATCH_GC() {\
    if (markRootSet)\
        MarkRootSet();\
    if (safePointRequested)\
        GetHeap<HEAP_CLS>()->SignalSafepointReached(&safePointRequested);\
    if (signalEnableGCTrap)\
        EnableGCTrap();\
    if (GetHeap<HEAP_CLS>()->IsPauseTriggered()) { \
        GetFrame()->SetBytecodeIndex(bytecodeIndexGlobal); \
        GetHeap<HEAP_CLS>()->Pause(); \
        method = GetFrame()->GetMethod(); \
        currentBytecodes = method->GetBytecodes(); \
    } \
    goto *loopTargets[currentBytecodes[bytecodeIndexGlobal]];\
}
#else
#define DISPATCH_GC() {\
  if (GetHeap<HEAP_CLS>()->isCollectionTriggered()) {\
    GetHeap<HEAP_CLS>()->FullGC();\
  }\
  goto *loopTargets[currentBytecodes[bytecodeIndexGlobal]];\
}
#endif

void Interpreter::Start() {
    // initialization
    method = GetFrame()->GetMethod();
    currentBytecodes = method->GetBytecodes();

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
      PAUSELESS_ONLY(EnableStop());
      return; // handle the halt bytecode

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

VMFrame* Interpreter::PushNewFrame(VMMethod* method) {
    SetFrame(GetUniverse()->NewFrame(GetFrame(), method, page));
    return GetFrame();
}

void Interpreter::SetFrame(VMFrame* frame) {
    if (this->frame != nullptr) {
        load_ptr(this->frame)->SetBytecodeIndex(bytecodeIndexGlobal);
    }

    this->frame = to_gc_ptr(frame);

    // update cached values
    method              = frame->GetMethod();
    bytecodeIndexGlobal = frame->GetBytecodeIndex();
    currentBytecodes    = method->GetBytecodes();
}

vm_oop_t Interpreter::GetSelf() {
    VMFrame* context = GetFrame()->GetOuterContext();
    return context->GetArgument(0, 0);
}

VMFrame* Interpreter::popFrame() {
    VMFrame* result = GetFrame();
    SetFrame(GetFrame()->GetPreviousFrame());

    result->ClearPreviousFrame();

#ifdef UNSAFE_FRAME_OPTIMIZATION
    //remember this frame as free frame
    result->GetMethod()->SetCachedFrame(result);
# error not to be used in multithreaded mode
#endif
    return result;
}

void Interpreter::popFrameAndPushResult(vm_oop_t result) {
    VMFrame* prevFrame = popFrame();

    VMMethod* method = prevFrame->GetMethod();
    long numberOfArgs = method->GetNumberOfArguments();

    for (long i = 0; i < numberOfArgs; ++i) GetFrame()->Pop();

    GetFrame()->Push(result);
}

void Interpreter::send(VMSymbol* signature, VMClass* receiverClass) {
    assert(Universe::IsValidObject(receiverClass));
    VMInvokable* invokable = receiverClass->LookupInvokable(signature);

    if (invokable != nullptr) {
#ifdef LOG_RECEIVER_TYPES
        StdString name = receiverClass->GetName()->GetStdString();
        if (GetUniverse()->callStats.find(name) == GetUniverse()->callStats.end())
        GetUniverse()->callStats[name] = {0,0};
        GetUniverse()->callStats[name].noCalls++;
        if (invokable->IsPrimitive())
        GetUniverse()->callStats[name].noPrimitiveCalls++;
#endif
        // since an invokable is able to change/use the frame, we have to write
        // cached values before, and read cached values after calling
        GetFrame()->SetBytecodeIndex(bytecodeIndexGlobal);
        invokable->Invoke(this, GetFrame());
        bytecodeIndexGlobal = GetFrame()->GetBytecodeIndex();
    } else {
        //doesNotUnderstand
        long numberOfArgs = Signature::GetNumberOfArguments(signature);

        vm_oop_t receiver = GetFrame()->GetStackElement(numberOfArgs-1);

        VMArray* argumentsArray = GetUniverse()->NewArray(numberOfArgs - 1, page); // without receiver

        // the receiver should not go into the argumentsArray
        // so, numberOfArgs - 2
        for (long i = numberOfArgs - 2; i >= 0; --i) {
            vm_oop_t o = GetFrame()->Pop();
            argumentsArray->SetIndexableField(i, o);
        }
        vm_oop_t arguments[] = {signature, argumentsArray};
        
        GetFrame()->Pop(); // pop the receiver

        //check if current frame is big enough for this unplanned Send
        //doesNotUnderstand: needs 3 slots, one for this, one for method name, one for args
        long additionalStackSlots = 3 - GetFrame()->RemainingStackSize();
        if (additionalStackSlots > 0) {
            GetFrame()->SetBytecodeIndex(bytecodeIndexGlobal);
            //copy current frame into a bigger one and replace the current frame
            SetFrame(VMFrame::EmergencyFrameFrom(GetFrame(), additionalStackSlots, page));
        }

        AS_OBJ(receiver)->Send(this, doesNotUnderstand, arguments, 2);
    }
}

void Interpreter::doDup() {
    vm_oop_t elem = GetFrame()->GetStackElement(0);
    GetFrame()->Push(elem);
}

void Interpreter::doPushLocal(long bytecodeIndex) {
    uint8_t bc1 = method->GetBytecode(bytecodeIndex + 1);
    uint8_t bc2 = method->GetBytecode(bytecodeIndex + 2);

    vm_oop_t local = GetFrame()->GetLocal(bc1, bc2);

    GetFrame()->Push(local);
}

void Interpreter::doPushArgument(long bytecodeIndex) {
    uint8_t bc1 = method->GetBytecode(bytecodeIndex + 1);
    uint8_t bc2 = method->GetBytecode(bytecodeIndex + 2);

    vm_oop_t argument = GetFrame()->GetArgument(bc1, bc2);

    GetFrame()->Push(argument);
}

void Interpreter::doPushField(long bytecodeIndex) {
    uint8_t fieldIndex = method->GetBytecode(bytecodeIndex + 1);
    vm_oop_t self = GetSelf();
    vm_oop_t o;
    
    assert(Universe::IsValidObject(self));

    if (unlikely(IS_TAGGED(self))) {
        o = nullptr;
        GetUniverse()->ErrorExit("Integers do not have fields!");
    }
    else {
        o = ((VMObject*)self)->GetField(fieldIndex);
    }
    
    assert(Universe::IsValidObject(o));

    GetFrame()->Push(o);
}

void Interpreter::doPushBlock(long bytecodeIndex) {
    // Short cut the negative case of #ifTrue: and #ifFalse:
    if (currentBytecodes[bytecodeIndexGlobal] == BC_SEND) {
        if (GetFrame()->GetStackElement(0) == load_ptr(falseObject) &&
            method->GetConstant(bytecodeIndexGlobal) == load_ptr(symbolIfTrue)) {
            GetFrame()->Push(load_ptr(nilObject));
            return;
        } else if (GetFrame()->GetStackElement(0) == load_ptr(trueObject) &&
                   method->GetConstant(bytecodeIndexGlobal) == load_ptr(symbolIfFalse)) {
            GetFrame()->Push(load_ptr(nilObject));
            return;
        }
    }

    VMMethod* blockMethod = static_cast<VMMethod*>(method->GetConstant(bytecodeIndex));

    long numOfArgs = blockMethod->GetNumberOfArguments();

    GetFrame()->Push(GetUniverse()->NewBlock(blockMethod, GetFrame(), numOfArgs, page));
}

void Interpreter::doPushConstant(long bytecodeIndex) {
    vm_oop_t constant = method->GetConstant(bytecodeIndex);
    GetFrame()->Push(constant);
}

void Interpreter::doPushGlobal(long bytecodeIndex) {
    VMSymbol* globalName = static_cast<VMSymbol*>(method->GetConstant(bytecodeIndex));
    vm_oop_t global = GetUniverse()->GetGlobal(globalName);

    if (global != nullptr)
        GetFrame()->Push(global);
    else {
        vm_oop_t arguments[] = {globalName};
        vm_oop_t self = GetSelf();

        //check if there is enough space on the stack for this unplanned Send
        //unknowGlobal: needs 2 slots, one for "this" and one for the argument
        long additionalStackSlots = 2 - GetFrame()->RemainingStackSize();
        if (additionalStackSlots > 0) {
            GetFrame()->SetBytecodeIndex(bytecodeIndexGlobal);
            //copy current frame into a bigger one and replace the current frame
            SetFrame(VMFrame::EmergencyFrameFrom(GetFrame(), additionalStackSlots, page));
        }

        AS_OBJ(self)->Send(this, unknownGlobal, arguments, 1);
    }
}

void Interpreter::doPop() {
    GetFrame()->Pop();
}

void Interpreter::doPopLocal(long bytecodeIndex) {
    uint8_t bc1 = method->GetBytecode(bytecodeIndex + 1);
    uint8_t bc2 = method->GetBytecode(bytecodeIndex + 2);

    vm_oop_t o = GetFrame()->Pop();
    assert(Universe::IsValidObject(o));

    GetFrame()->SetLocal(bc1, bc2, o);
}

void Interpreter::doPopArgument(long bytecodeIndex) {
    uint8_t bc1 = method->GetBytecode(bytecodeIndex + 1);
    uint8_t bc2 = method->GetBytecode(bytecodeIndex + 2);

    vm_oop_t o = GetFrame()->Pop();
    assert(Universe::IsValidObject(o));
    GetFrame()->SetArgument(bc1, bc2, o);
}

void Interpreter::doPopField(long bytecodeIndex) {
    uint8_t field_index = method->GetBytecode(bytecodeIndex + 1);

    vm_oop_t self = GetSelf();
    vm_oop_t o = GetFrame()->Pop();
    
    assert(Universe::IsValidObject(self));
    assert(Universe::IsValidObject(o));

    if (unlikely(IS_TAGGED(self))) {
        GetUniverse()->ErrorExit("Integers do not have fields that can be set");
    }
    else {
        static_cast<VMObject*>(self)->SetField(field_index, o);
    }
}

void Interpreter::doSend(long bytecodeIndex) {
    VMSymbol* signature = static_cast<VMSymbol*>(method->GetConstant(bytecodeIndex));

    int numOfArgs = Signature::GetNumberOfArguments(signature);

    vm_oop_t receiver = GetFrame()->GetStackElement(numOfArgs-1);
    assert(Universe::IsValidObject(receiver));
    assert(dynamic_cast<VMClass*>(CLASS_OF(receiver)) != nullptr); // make sure it is really a class
    
    VMClass* receiverClass = CLASS_OF(receiver);
    
    assert(Universe::IsValidObject(receiverClass));

#ifdef LOG_RECEIVER_TYPES
    GetUniverse()->receiverTypes[receiverClass->GetName()->GetStdString()]++;
#endif

    send(signature, receiverClass);
}

void Interpreter::doSuperSend(long bytecodeIndex) {
    VMSymbol* signature = static_cast<VMSymbol*>(method->GetConstant(bytecodeIndex));

    VMFrame* ctxt = GetFrame()->GetOuterContext();
    VMMethod* realMethod = ctxt->GetMethod();
    VMClass* holder = realMethod->GetHolder();
    VMClass* super = holder->GetSuperClass();
    VMInvokable* invokable = static_cast<VMInvokable*>(super->LookupInvokable(signature));

    if (invokable != nullptr)
        invokable->Invoke(this, GetFrame());
    else {
        long numOfArgs = Signature::GetNumberOfArguments(signature);
        vm_oop_t receiver = GetFrame()->GetStackElement(numOfArgs - 1);
        VMArray* argumentsArray = GetUniverse()->NewArray(numOfArgs, page);

        for (long i = numOfArgs - 1; i >= 0; --i) {
            vm_oop_t o = GetFrame()->Pop();
            argumentsArray->SetIndexableField(i, o);
        }
        vm_oop_t arguments[] = {signature, argumentsArray};

        AS_OBJ(receiver)->Send(this, doesNotUnderstand, arguments, 2);
    }
}

void Interpreter::doReturnLocal() {
    vm_oop_t result = GetFrame()->Pop();
    popFrameAndPushResult(result);
}

void Interpreter::doReturnNonLocal() {
    vm_oop_t result = GetFrame()->Pop();

    VMFrame* context = GetFrame()->GetOuterContext();

    if (!context->HasPreviousFrame()) {
        VMBlock* block = static_cast<VMBlock*>(GetFrame()->GetArgument(0, 0));
        VMFrame* prevFrame = GetFrame()->GetPreviousFrame();
        VMFrame* outerContext = prevFrame->GetOuterContext();
        vm_oop_t sender = outerContext->GetArgument(0, 0);
        vm_oop_t arguments[] = {block};

        popFrame();
        
        // Pop old arguments from stack
        VMMethod* method = GetFrame()->GetMethod();
        long numberOfArgs = method->GetNumberOfArguments();
        for (long i = 0; i < numberOfArgs; ++i)
            GetFrame()->Pop();

        // check if current frame is big enough for this unplanned send
        // #escapedBlock: needs 2 slots, one for self, and one for the block
        long additionalStackSlots = 2 - GetFrame()->RemainingStackSize();
        if (additionalStackSlots > 0) {
            GetFrame()->SetBytecodeIndex(bytecodeIndexGlobal);
            // copy current frame into a bigger one, and replace it
            SetFrame(VMFrame::EmergencyFrameFrom(GetFrame(), additionalStackSlots, page));
        }

        AS_OBJ(sender)->Send(this, escapedBlock, arguments, 1);
        return;
    }

    while (GetFrame() != context) popFrame();

    popFrameAndPushResult(result);
}

void Interpreter::doJumpIfFalse(long bytecodeIndex) {
    vm_oop_t value = GetFrame()->Pop();
    if (value == load_ptr(falseObject))
        doJump(bytecodeIndex);
}

void Interpreter::doJumpIfTrue(long bytecodeIndex) {
    vm_oop_t value = GetFrame()->Pop();
    if (value == load_ptr(trueObject))
        doJump(bytecodeIndex);
}

void Interpreter::doJump(long bytecodeIndex) {
    long target = 0;
    target |= method->GetBytecode(bytecodeIndex + 1);
    target |= method->GetBytecode(bytecodeIndex + 2) << 8;
    target |= method->GetBytecode(bytecodeIndex + 3) << 16;
    target |= method->GetBytecode(bytecodeIndex + 4) << 24;

    // do the jump
    bytecodeIndexGlobal = target;
}

#if GC_TYPE==PAUSELESS
// Request a marking of the interpreters' root set
void Interpreter::TriggerMarkRootSet() {
    pthread_mutex_lock(&blockedMutex);
    if (blocked)
        GetHeap<HEAP_CLS>()->SignalInterpreterBlocked(this);
    else if (stopped)
        GetHeap<HEAP_CLS>()->SignalRootSetMarked();
    else
        markRootSet = true;
    pthread_mutex_unlock(&blockedMutex);
}

// The interpreter is able to mark its root set himself
void Interpreter::MarkRootSet() {
    markRootSet = false;
    expectedNMT = !expectedNMT;
    worklist.Clear();
    
    // this will also destructively change the frame and method pointers so that the NMT bit is flipped
    ReadBarrier(&frame, true);
    // ReadBarrier(&method);
    
    while (!fullPages.empty()) {
#warning TODO: see what's going on here, are we sure that the GC has only access to full pages?

        GetHeap<HEAP_CLS>()->RelinquishPage(fullPages.back());
        fullPages.pop_back();
    }
    
    // signal that root-set has been marked
    GetHeap<HEAP_CLS>()->SignalRootSetMarked();
    
    //GetHeap<HEAP_CLS>()->TriggerPause();
    //GetHeap<HEAP_CLS>()->Pause();
}

// The interpreter is unable to mark its root set himself and thus one of the gc threads does it
void Interpreter::MarkRootSetByGC() {
    markRootSet = false;
    expectedNMT = !expectedNMT;
    worklist.Clear();
    
    // this will also destructively change the frame and method pointers so that the NMT bit is flipped
    ReadBarrierForGCThread(&frame, true);
    // ReadBarrierForGCThread(&method);
    
    while (!fullPages.empty()) {
        //fullPages.back().ResetAmountOfLiveData();
        GetHeap<HEAP_CLS>()->RelinquishPage(fullPages.back());
        fullPages.pop_back();
    }
    
    // signal that root-set has been marked
    GetHeap<HEAP_CLS>()->SignalRootSetMarked();
    
    //GetHeap<HEAP_CLS>()->TriggerPause();
    //GetHeap<HEAP_CLS>()->PauseGC();
}

// Request that the mutator thread passes a safepoint so that marking can finish
void Interpreter::RequestSafePoint() {
    // if test to prevent deadlocking in case safePointRequesting was still true and the mutator thread is performing an enableBlocked
    if (!safePointRequested) {
        pthread_mutex_lock(&blockedMutex);
        if (blocked || stopped)
            GetHeap<HEAP_CLS>()->SignalSafepointReached(&safePointRequested);
        else
            safePointRequested = true;
        pthread_mutex_unlock(&blockedMutex);
    }
}

// Request that the mutator thread enables its GC-trap
void Interpreter::SignalEnableGCTrap() {
    pthread_mutex_lock(&blockedMutex);
    if (blocked || stopped) {
        gcTrapEnabled = true;
        GetHeap<HEAP_CLS>()->SignalGCTrapEnabled();
    } else
        signalEnableGCTrap = true;
    pthread_mutex_unlock(&blockedMutex);
}

// Switch the GC-trap on when in a safepoint and notify the collector of the fact that the trap is switched on
void Interpreter::EnableGCTrap() {
    signalEnableGCTrap = false;
    gcTrapEnabled = true;
    GetHeap<HEAP_CLS>()->SignalGCTrapEnabled();
}

// Switch the GC-trap off again, this does not require a safepoint pass
void Interpreter::DisableGCTrap() {
    prevent.lock();
    gcTrapEnabled = false;
    prevent.unlock();
}

// used when interpreter is in the process of going into a blocked state
void Interpreter::EnableBlocked() {
    pthread_mutex_lock(&blockedMutex);
    if (markRootSet)
        MarkRootSet();
    if (safePointRequested)
        GetHeap<HEAP_CLS>()->SignalSafepointReached(&safePointRequested);
    if (signalEnableGCTrap)
        EnableGCTrap();
    blocked = true;
    pthread_mutex_unlock(&blockedMutex);
}

// Used when interpreter gets out of a blocked state
void Interpreter::DisableBlocked() {
    blocked = false;
}

// used when interpreter is in the processes of halting
void Interpreter::EnableStop() {
    pthread_mutex_lock(&blockedMutex);
    if (markRootSet)
        GetHeap<HEAP_CLS>()->SignalRootSetMarked();
    if (safePointRequested)
        GetHeap<HEAP_CLS>()->SignalSafepointReached(&safePointRequested);
    if (signalEnableGCTrap)
        GetHeap<HEAP_CLS>()->SignalGCTrapEnabled();
    stopped = true;
    pthread_mutex_unlock(&blockedMutex);
}

// methods used by the read barrier
void Interpreter::AddGCWork(AbstractVMObject* work) {
    worklist.AddWorkMutator(work);
}

bool Interpreter::GCTrapEnabled() {
    return gcTrapEnabled;
}

//new --->

/// The GC trap is to make sure that a page is blocked atomically, and
//  not observable by a mutator. The mutator are not allowed to see the
//  blocking change within the period of two safepoints, because otherwise
//  they could be seeing two different pointers for the same object

bool Interpreter::TriggerGCTrap(Page* page) {
    prevent.lock();
    bool result = gcTrapEnabled && page->Blocked();
    prevent.unlock();
    return result;
}
// <-----

// page management
void Interpreter::AddFullPage(Page* page) {
    fullPages.push_back(page);
}

// debug procedures
void Interpreter::CheckMarking(void (*walk)(vm_oop_t)) {
    // VMMethod* testMethodGCSet = Untag(method);
    if (frame) {
        //assert(GetNMTValue(frame) == GetHeap<HEAP_CLS>()->GetGCThread()->GetExpectedNMT());
        walk(Untag(frame));
    }
}
#endif

void Interpreter::WalkGlobals(walk_heap_fn walk, Page* page) {
    // some barriers need a field to work on, so use a temporary
    GCMethod* m = to_gc_ptr(method);
    m = static_cast<GCMethod*>(walk(m, page));
    method = load_ptr(m); // could have moved, for instance with generational GC
    currentBytecodes = method->GetBytecodes();

    // Get the current frame and mark it.
    // Since marking is done recursively, this automatically
    // marks the whole stack
    frame  = static_cast<GCFrame*>(walk(frame, page));
}
