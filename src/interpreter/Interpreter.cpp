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
#include "bytecodes.h"

#if GC_TYPE==PAUSELESS
    #include "../memory/pauseless/PauselessCollector.h"
#endif

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
#include <natives/VMThread.h>
#ifdef USE_TAGGING
#include <vmobjects/IntegerBox.h>
#endif

#include <compiler/Disassembler.h>
#include <vmobjects/VMBlock.inline.h>
#include <vmobjects/VMMethod.inline.h>

const StdString Interpreter::unknownGlobal     = "unknownGlobal:";
const StdString Interpreter::doesNotUnderstand = "doesNotUnderstand:arguments:";
const StdString Interpreter::escapedBlock      = "escapedBlock:";


#if GC_TYPE==PAUSELESS
Interpreter::Interpreter(bool expectedNMT, bool gcTrapEnabled) : BaseThread(expectedNMT) {
    this->thread = nullptr;
    this->frame = nullptr;

    stopped = false;
    blocked = false;
    markRootSet = false;
    safePointRequested = false;
    signalEnableGCTrap = false;
    this->gcTrapEnabled = gcTrapEnabled;
    //this->gcTrapEnabled = false;
    nonRelocatablePage = _HEAP->RequestPage();
    fullPages = vector<Page*>();
    nonRelocatablePages = vector<Page*>();
    pthread_mutex_init(&blockedMutex, nullptr);
}
#else
Interpreter::Interpreter() : BaseThread() {
    this->thread = nullptr;
    this->frame = nullptr;
}
#endif

//Interpreter::~Interpreter() {
    /*while (!fullPages.empty()) {
        _HEAP->RelinquishPage(fullPages.back());
        fullPages.pop_back();
    } */
//}

#define PROLOGUE(bc_count) {\
  if (dumpBytecodes > 1) Disassembler::DumpBytecode(GetFrame(), GetFrame()->GetMethod(), bytecodeIndexGlobal);\
  bytecodeIndexGlobal += bc_count;\
}

#define DISPATCH_NOGC() {\
  goto *loopTargets[/* currentBytecodes */ _FRAME->GetMethod()->GetBytecodes()[bytecodeIndexGlobal]]; \
}

#if GC_TYPE==PAUSELESS
#define DISPATCH_GC() {\
    if (markRootSet)\
        MarkRootSet();\
    if (safePointRequested)\
        _HEAP->SignalSafepointReached(&safePointRequested);\
    if (signalEnableGCTrap)\
        EnableGCTrap();\
    if (_HEAP->IsPauseTriggered()) { \
        GetFrame()->SetBytecodeIndex(bytecodeIndexGlobal); \
        _HEAP->Pause(); \
        /* method = _FRAME->GetMethod(); */ \
        /* currentBytecodes = method->GetBytecodes(); */ \
    } \
    goto *loopTargets[/* currentBytecodes */ _FRAME->GetMethod()->GetBytecodes()[bytecodeIndexGlobal]];\
}
#else
#define DISPATCH_GC() {\
  if (_HEAP->isCollectionTriggered()) {\
    GetFrame()->SetBytecodeIndex(bytecodeIndexGlobal);\
    _HEAP->FullGC();\
    /* method = _FRAME->GetMethod();*/ \
    /* currentBytecodes = method->GetBytecodes(); */ \
  }\
  goto *loopTargets[/* currentBytecodes */ _FRAME->GetMethod()->GetBytecodes() [bytecodeIndexGlobal]];\
}
#endif


void Interpreter::Start() {
    // initialization

    // method = WRITEBARRIER(_FRAME->GetMethod());
    // currentBytecodes = method->GetBytecodes();

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

    goto *loopTargets[/* currentBytecodes */ _FRAME->GetMethod()->GetBytecodes()[bytecodeIndexGlobal]];

    //
    // THIS IS THE former interpretation loop
    LABEL_BC_HALT:
      PROLOGUE(1);
#if GC_TYPE==PAUSELESS
    EnableStop();
#endif
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
    SetFrame(GetUniverse()->NewFrame(GetFrame(), method));
    return GetFrame();
}

void Interpreter::SetFrame(VMFrame* frame) {
    if (READBARRIER(this->frame) != nullptr) {
        READBARRIER(this->frame)->SetBytecodeIndex(bytecodeIndexGlobal);
    }

    this->frame = WRITEBARRIER(frame);

    // update cached values
    // method              = WRITEBARRIER(frame->GetMethod());
    bytecodeIndexGlobal = frame->GetBytecodeIndex();
    // currentBytecodes    = READBARRIER(method)->GetBytecodes();
}

VMFrame* Interpreter::GetFrame() {
    return READBARRIER(this->frame);
}

vm_oop_t Interpreter::GetSelf() {
    VMFrame* context = GetFrame()->GetOuterContext();
    return context->GetArgument(0, 0);
}

VMFrame* Interpreter::popFrame() {
    VMFrame* result = GetFrame();
    SetFrame(GetFrame()->GetPreviousFrame());

    result->ClearPreviousFrame();

    /*
#ifdef UNSAFE_FRAME_OPTIMIZATION
    //remember this frame as free frame
    result->GetMethod()->SetCachedFrame(result);
#endif
    */
    
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
    //sync_out(ostringstream() << "[Send] " << signature->GetChars());
    
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
        (*invokable)(GetFrame());
        bytecodeIndexGlobal = GetFrame()->GetBytecodeIndex();
    } else {
        //doesNotUnderstand
        long numberOfArgs = Signature::GetNumberOfArguments(signature);

        vm_oop_t receiver = GetFrame()->GetStackElement(numberOfArgs-1);

        VMArray* argumentsArray = GetUniverse()->NewArray(numberOfArgs - 1); // without receiver

        for (long i = numberOfArgs - 1; i >= 0; --i) {
            vm_oop_t o = GetFrame()->Pop();
            argumentsArray->SetIndexableField(i, o);
        }
        vm_oop_t arguments[] = {signature, argumentsArray};

        //check if current frame is big enough for this unplanned Send
        //doesNotUnderstand: needs 3 slots, one for this, one for method name, one for args
        long additionalStackSlots = 3 - _FRAME->RemainingStackSize();
        if (additionalStackSlots > 0) {
            //copy current frame into a bigger one and replace the current frame
            this->SetFrame(VMFrame::EmergencyFrameFrom(_FRAME, additionalStackSlots));
        }

        AS_OBJ(receiver)->Send(doesNotUnderstand, arguments, 2);
    }
}

void Interpreter::doDup() {
    vm_oop_t elem = GetFrame()->GetStackElement(0);
    GetFrame()->Push(elem);
}

void Interpreter::doPushLocal(long bytecodeIndex) {
    //VMMethod* method = this->GetMethod();
    uint8_t bc1 = GetMethod()->GetBytecode(bytecodeIndex + 1);
    uint8_t bc2 = GetMethod()->GetBytecode(bytecodeIndex + 2);

    vm_oop_t local = GetFrame()->GetLocal(bc1, bc2);

    GetFrame()->Push(local);
}

void Interpreter::doPushArgument(long bytecodeIndex) {
    //VMMethod* method = this->GetMethod();
    uint8_t bc1 = GetMethod()->GetBytecode(bytecodeIndex + 1);
    uint8_t bc2 = GetMethod()->GetBytecode(bytecodeIndex + 2);

    vm_oop_t argument = GetFrame()->GetArgument(bc1, bc2);

    GetFrame()->Push(argument);
}

void Interpreter::doPushField(long bytecodeIndex) {
    uint8_t fieldIndex = GetMethod()->GetBytecode(bytecodeIndex + 1);
    vm_oop_t self = GetSelf();
    vm_oop_t o;
    
    assert(Universe::IsValidObject(self));

    if (unlikely(IS_TAGGED(self))) {
        o = nullptr;
        Universe()->ErrorExit("Integers do not have fields!");
    }
    else {
        o = ((VMObject*)self)->GetField(fieldIndex);
    }
    
    assert(Universe::IsValidObject(o));

    GetFrame()->Push(o);
}

void Interpreter::doPushBlock(long bytecodeIndex) {
    // Short cut the negative case of #ifTrue: and #ifFalse:
    if (/* currentBytecodes */ _FRAME->GetMethod()->GetBytecodes()[bytecodeIndexGlobal] == BC_SEND) {
        if (_FRAME->GetStackElement(0) == READBARRIER(falseObject) &&
            this->GetMethod()->GetConstant(bytecodeIndexGlobal) == READBARRIER(symbolIfTrue)) {
            _FRAME->Push(READBARRIER(nilObject));
            return;
        }
        if (_FRAME->GetStackElement(0) == READBARRIER(trueObject) &&
            this->GetMethod()->GetConstant(bytecodeIndexGlobal) == READBARRIER(symbolIfFalse)) {
            _FRAME->Push(READBARRIER(nilObject));
            return;
        }
    }

    VMMethod* blockMethod = static_cast<VMMethod*>(this->GetMethod()->GetConstant(bytecodeIndex));
    
    long numOfArgs = blockMethod->GetNumberOfArguments();

    _FRAME->Push(GetUniverse()->NewBlock(blockMethod, _FRAME, numOfArgs));
}

void Interpreter::doPushConstant(long bytecodeIndex) {
    VMObject* constant = this->GetMethod()->GetConstant(bytecodeIndex);
    _FRAME->Push(constant);
}

void Interpreter::doPushGlobal(long bytecodeIndex) {
    VMSymbol* globalName = static_cast<VMSymbol*>(this->GetMethod()->GetConstant(bytecodeIndex));
    VMObject* global = GetUniverse()->GetGlobal(globalName);

    if(global != NULL)
        _FRAME->Push(global);
    else {
        VMObject* arguments[] = {globalName};
        VMObject* self = _SELF;

        //check if there is enough space on the stack for this unplanned Send
        //unknowGlobal: needs 2 slots, one for "this" and one for the argument
        long additionalStackSlots = 2 - _FRAME->RemainingStackSize();
        if (additionalStackSlots > 0) {
            _FRAME->SetBytecodeIndex(bytecodeIndexGlobal);
            //copy current frame into a bigger one and replace the current frame
            this->SetFrame(VMFrame::EmergencyFrameFrom(_FRAME,
                            additionalStackSlots));
        }

        AS_OBJ(self)->Send(unknownGlobal, arguments, 1);
    }
}

void Interpreter::doPop() {
    _FRAME->Pop();
}

void Interpreter::doPopLocal(long bytecodeIndex) {
    //VMMethod* method = this->GetMethod();
    uint8_t bc1 = this->GetMethod()->GetBytecode(bytecodeIndex + 1);
    uint8_t bc2 = this->GetMethod()->GetBytecode(bytecodeIndex + 2);

    VMObject* o = _FRAME->Pop();

    _FRAME->SetLocal(bc1, bc2, o);
}

void Interpreter::doPopArgument(long bytecodeIndex) {
    //VMMethod* method = this->GetMethod();
    uint8_t bc1 = this->GetMethod()->GetBytecode(bytecodeIndex + 1);
    uint8_t bc2 = this->GetMethod()->GetBytecode(bytecodeIndex + 2);

    VMObject* o = _FRAME->Pop();
    _FRAME->SetArgument(bc1, bc2, o);
}

void Interpreter::doPopField(long bytecodeIndex) {
    uint8_t field_index = this->GetMethod()->GetBytecode(bytecodeIndex + 1);

    VMObject* self = _SELF;
    VMObject* o = _FRAME->Pop();
#ifdef USE_TAGGING
    if (IS_TAGGED(self)) {
        GlobalBox::IntegerBox()->SetField(field_index, o);
    }
    else {
        AS_POINTER(self)->SetField(field_index, o);
    }
#else
    static_cast<VMObject*>(self)->SetField(field_index, o);
#endif
}

void Interpreter::doSend(long bytecodeIndex) {
    VMSymbol* signature = static_cast<VMSymbol*>(this->GetMethod()->GetConstant(bytecodeIndex));

    long numOfArgs = Signature::GetNumberOfArguments(signature);

    VMObject* receiver = _FRAME->GetStackElement(numOfArgs-1);
    assert(Universe::IsValidObject(receiver));
    assert(dynamic_cast<VMClass*>((VMObject*)receiver->GetClass()) != nullptr); // make sure it is really a class
    
#ifdef USE_TAGGING
    VMClass* receiverClass = IS_TAGGED(receiver) ? integerClass : AS_POINTER(receiver)->GetClass();
#else
    VMClass* receiverClass = receiver->GetClass();
#endif
    
    assert(Universe::IsValidObject(receiverClass));

#ifdef LOG_RECEIVER_TYPES
    GetUniverse()->receiverTypes[receiverClass->GetName()->GetStdString()]++;
#endif

    this->send(signature, receiverClass);
}

void Interpreter::doSuperSend(long bytecodeIndex) {
    VMSymbol* signature = static_cast<VMSymbol*>(this->GetMethod()->GetConstant(bytecodeIndex));

    VMFrame* ctxt = _FRAME->GetOuterContext();
    VMMethod* realMethod = ctxt->GetMethod();
    VMClass* holder = realMethod->GetHolder();
    VMClass* super = holder->GetSuperClass();
    VMInvokable* invokable = static_cast<VMInvokable*>(super->LookupInvokable(signature));

    if (invokable != NULL)
        (*invokable)(_FRAME);
    else {
        long numOfArgs = Signature::GetNumberOfArguments(signature);
        VMObject* receiver = _FRAME->GetStackElement(numOfArgs - 1);
        VMArray* argumentsArray = GetUniverse()->NewArray(numOfArgs);

        for (long i = numOfArgs - 1; i >= 0; --i) {
            VMObject* o = _FRAME->Pop();
            argumentsArray->SetIndexableField(i, o);
        }
        VMObject* arguments[] = {signature, argumentsArray};
#ifdef USE_TAGGING
        if (IS_TAGGED(receiver))
            GlobalBox::IntegerBox()->Send(dnu, arguments, 2);
        else
            AS_POINTER(receiver)->Send(dnu, arguments, 2);
#else
        receiver->Send(dnu, arguments, 2);
#endif
    }
}

void Interpreter::doReturnLocal() {
    VMObject* result = _FRAME->Pop();

    this->popFrameAndPushResult(result);
}

void Interpreter::doReturnNonLocal() {
    VMObject* result = _FRAME->Pop();

    VMFrame* context = _FRAME->GetOuterContext();

    if (!context->HasPreviousFrame()) {
        VMBlock* block = static_cast<VMBlock*>(_FRAME->GetArgument(0, 0));
        VMFrame* prevFrame = _FRAME->GetPreviousFrame();
        VMFrame* outerContext = prevFrame->GetOuterContext();
        VMObject* sender = outerContext->GetArgument(0, 0);
        VMObject* arguments[] = {block};

        this->popFrame();

#ifdef USE_TAGGING
        if (IS_TAGGED(sender))
            GlobalBox::IntegerBox()->Send(eB, arguments, 1);
        else
            AS_POINTER(sender)->Send(eB, arguments, 1);
#else
        sender->Send(eB, arguments, 1);
#endif
        return;
    }

    while (_FRAME != context) this->popFrame();

    this->popFrameAndPushResult(result);
}

void Interpreter::doJumpIfFalse(long bytecodeIndex) {
    VMObject* value = _FRAME->Pop();
    if (value == READBARRIER(falseObject))
        doJump(bytecodeIndex);
}

void Interpreter::doJumpIfTrue(long bytecodeIndex) {
    VMObject* value = _FRAME->Pop();
    if (value == READBARRIER(trueObject))
        doJump(bytecodeIndex);
}

void Interpreter::doJump(long bytecodeIndex) {
    long target = 0;
    VMMethod* method = this->GetMethod();
    target |= method->GetBytecode(bytecodeIndex + 1);
    target |= method->GetBytecode(bytecodeIndex + 2) << 8;
    target |= method->GetBytecode(bytecodeIndex + 3) << 16;
    target |= method->GetBytecode(bytecodeIndex + 4) << 24;

    // do the jump
    bytecodeIndexGlobal = target;
}

VMMethod* Interpreter::GetMethod() {
    return GetFrame()->GetMethod();
    // return READBARRIER(this->method);
}

VMThread* Interpreter::GetThread(void) {
    return READBARRIER(this->thread);
}

void Interpreter::SetThread(VMThread* thread) {
    this->thread = WRITEBARRIER(thread);
}

#if GC_TYPE==PAUSELESS
// Request a marking of the interpreters' root set
void Interpreter::TriggerMarkRootSet() {
    pthread_mutex_lock(&blockedMutex);
    if (blocked)
        _HEAP->SignalInterpreterBlocked(this);
    else if (stopped)
        _HEAP->SignalRootSetMarked();
    else
        markRootSet = true;
    pthread_mutex_unlock(&blockedMutex);
}

// The interpreter is able to mark its root set himself
void Interpreter::MarkRootSet() {
    markRootSet = false;
    expectedNMT = !expectedNMT;
    worklist.Clear();
    
    // this will also destructively change the thread, frame and method pointers so that the NMT bit is flipped
    ReadBarrier(&thread, true);
    ReadBarrier(&frame, true);
    // ReadBarrier(&method);
    
    while (!fullPages.empty()) {
        //fullPages.back().ResetAmountOfLiveData();
        _HEAP->RelinquishPage(fullPages.back());
        fullPages.pop_back();
    }
    
    // signal that root-set has been marked
    _HEAP->SignalRootSetMarked();
    
    //_HEAP->TriggerPause();
    //_HEAP->Pause();
}

// The interpreter is unable to mark its root set himself and thus one of the gc threads does it
void Interpreter::MarkRootSetByGC() {
    markRootSet = false;
    expectedNMT = !expectedNMT;
    worklist.Clear();
    
    // this will also destructively change the thread, frame and method pointers so that the NMT bit is flipped
    ReadBarrierForGCThread(&thread, true);
    ReadBarrierForGCThread(&frame, true);
    // ReadBarrierForGCThread(&method);
    
    while (!fullPages.empty()) {
        //fullPages.back().ResetAmountOfLiveData();
        _HEAP->RelinquishPage(fullPages.back());
        fullPages.pop_back();
    }
    
    // signal that root-set has been marked
    _HEAP->SignalRootSetMarked();
    
    //_HEAP->TriggerPause();
    //_HEAP->PauseGC();
}

// Request that the mutator thread passes a safepoint so that marking can finish
void Interpreter::RequestSafePoint() {
    // if test to prevent deadlocking in case safePointRequesting was still true and the mutator thread is performing an enableBlocked
    if (!safePointRequested) {
        pthread_mutex_lock(&blockedMutex);
        if (blocked || stopped)
            _HEAP->SignalSafepointReached(&safePointRequested);
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
        _HEAP->SignalGCTrapEnabled();
    } else
        signalEnableGCTrap = true;
    pthread_mutex_unlock(&blockedMutex);
}

// Switch the GC-trap on when in a safepoint and notify the collector of the fact that the trap is switched on
void Interpreter::EnableGCTrap() {
    signalEnableGCTrap = false;
    gcTrapEnabled = true;
    _HEAP->SignalGCTrapEnabled();
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
        _HEAP->SignalSafepointReached(&safePointRequested);
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
        _HEAP->SignalRootSetMarked();
    if (safePointRequested)
        _HEAP->SignalSafepointReached(&safePointRequested);
    if (signalEnableGCTrap)
        _HEAP->SignalGCTrapEnabled();
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

Page* Interpreter::GetNonRelocatablePage() {
    return nonRelocatablePage;
}

void Interpreter::SetNonRelocatablePage(Page* page) {
    this->nonRelocatablePage = page;
}

void Interpreter::AddFullNonRelocatablePage(Page* page) {
    nonRelocatablePages.push_back(page);
}

// debug procedures
void Interpreter::CheckMarking(void (*walk)(AbstractVMObject*)) {
    // VMMethod* testMethodGCSet = Untag(method);
    if (frame) {
        //assert(GetNMTValue(frame) == _HEAP->GetGCThread()->GetExpectedNMT());
        walk(Untag(frame));
    }
    if (thread) {
        //assert(GetNMTValue(thread) == _HEAP->GetGCThread()->GetExpectedNMT());
        walk(Untag(thread));
    }
}


/*
 
 void Interpreter::CancelSafePoint() {
 safePointRequested = false;
 }
 
 // Since the interpreter is going to stop anyway it sufices to only signal the gc threads that the root set is marked without actually doing it
 void Interpreter::DummyMarkRootSet() {
 _HEAP->SignalRootSetMarked();
 }
 
 
 // Signal the fact that a safepoint is reached
 void Interpreter::SignalSafepointReached() {
 _HEAP->SignalSafepointReached();
 }
 
 */

#else
void Interpreter::WalkGlobals(VMOBJECT_PTR (*walk)(VMOBJECT_PTR)) {
    //method = (GCMethod*) walk(READBARRIER(method));
    thread = (GCThread*) walk(READBARRIER(thread));
    frame = (GCFrame*) walk(READBARRIER(frame));
}
#endif
