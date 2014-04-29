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

#include "Interpreter.h"
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
#ifdef USE_TAGGING
#include <vmobjects/IntegerBox.h>
#endif

#include <compiler/Disassembler.h>

// convenience macros for frequently used function invocations
#define _FRAME this->GetFrame()
#define _SELF this->GetSelf()

Interpreter::Interpreter() {
    this->thread = NULL;
    this->frame = NULL;
    
    this->page = _HEAP->RequestPage();

    uG = "unknownGlobal:";
    dnu = "doesNotUnderstand:arguments:";
    eB = "escapedBlock:";

}

Interpreter::~Interpreter() {

}

#define PROLOGUE(bc_count) {\
  if (dumpBytecodes > 1) Disassembler::DumpBytecode(_FRAME, _FRAME->GetMethod(), bytecodeIndexGlobal);\
  bytecodeIndexGlobal += bc_count;\
}

#define DISPATCH_NOGC() {\
  goto *loopTargets[currentBytecodes[bytecodeIndexGlobal]]; \
}

#define DISPATCH_GC() {\
  if (_HEAP->isCollectionTriggered()) {\
    _FRAME->SetBytecodeIndex(bytecodeIndexGlobal);\
    _HEAP->FullGC();\
    method = _FRAME->GetMethod(); \
    currentBytecodes = method->GetBytecodes(); \
  }\
  goto *loopTargets[currentBytecodes[bytecodeIndexGlobal]];\
}


void Interpreter::Start() {
    // initialization
    method = _FRAME->GetMethod();
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
}
;

goto *loopTargets[currentBytecodes[bytecodeIndexGlobal]];

//
// THIS IS THE former interpretation loop
LABEL_BC_HALT:
  PROLOGUE(1);
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

pVMFrame Interpreter::PushNewFrame(pVMMethod method) {
    SetFrame(_UNIVERSE->NewFrame(_FRAME, method));
    return _FRAME;
}

void Interpreter::SetFrame(pVMFrame frame) {
    if (this->frame != NULL)
        this->frame->SetBytecodeIndex(bytecodeIndexGlobal);

    this->frame = frame;

    // update cached values
    method              = frame->GetMethod();
    bytecodeIndexGlobal = frame->GetBytecodeIndex();
    currentBytecodes    = method->GetBytecodes();
}

pVMFrame Interpreter::GetFrame() {
    return this->frame;
}

pVMObject Interpreter::GetSelf() {
    pVMFrame context = _FRAME->GetOuterContext();
    return context->GetArgument(0,0);
}

pVMFrame Interpreter::popFrame() {
    pVMFrame result = _FRAME;
    this->SetFrame(_FRAME->GetPreviousFrame());

    result->ClearPreviousFrame();

    /*
#ifdef UNSAFE_FRAME_OPTIMIZATION
    //remember this frame as free frame
    result->GetMethod()->SetCachedFrame(result);
#endif
    */
    
    return result;
}

void Interpreter::popFrameAndPushResult(pVMObject result) {
    pVMFrame prevFrame = this->popFrame();

    pVMMethod method = prevFrame->GetMethod();
    long numberOfArgs = method->GetNumberOfArguments();

    for (long i = 0; i < numberOfArgs; ++i) _FRAME->Pop();

    _FRAME->Push(result);
}

void Interpreter::send(pVMSymbol signature, pVMClass receiverClass) {
    
    pVMInvokable invokable = receiverClass->LookupInvokable(signature);

    if (invokable != NULL) {
#ifdef LOG_RECEIVER_TYPES
        StdString name = receiverClass->GetName()->GetStdString();
        if (_UNIVERSE->callStats.find(name) == _UNIVERSE->callStats.end())
        _UNIVERSE->callStats[name] = {0,0};
        _UNIVERSE->callStats[name].noCalls++;
        if (invokable->IsPrimitive())
        _UNIVERSE->callStats[name].noPrimitiveCalls++;
#endif
        // since an invokable is able to change/use the frame, we have to write
        // cached values before, and read cached values after calling
        _FRAME->SetBytecodeIndex(bytecodeIndexGlobal);
        (*invokable)(_FRAME);
        bytecodeIndexGlobal = _FRAME->GetBytecodeIndex();
    } else {
        //doesNotUnderstand
        long numberOfArgs = Signature::GetNumberOfArguments(signature);

        pVMObject receiver = _FRAME->GetStackElement(numberOfArgs-1);

        pVMArray argumentsArray = _UNIVERSE->NewArray(numberOfArgs);

        for (long i = numberOfArgs - 1; i >= 0; --i) {
            pVMObject o = _FRAME->Pop();
            argumentsArray->SetIndexableField(i, o);
        }
        pVMObject arguments[] = {signature, argumentsArray};

        //check if current frame is big enough for this unplanned Send
        //doesNotUnderstand: needs 3 slots, one for this, one for method name, one for args
        long additionalStackSlots = 3 - _FRAME->RemainingStackSize();
        if (additionalStackSlots > 0) {
            //copy current frame into a bigger one and replace the current frame
            this->SetFrame(VMFrame::EmergencyFrameFrom(_FRAME, additionalStackSlots));
        }

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

void Interpreter::doDup() {
    pVMObject elem = _FRAME->GetStackElement(0);
    _FRAME->Push(elem);
}

void Interpreter::doPushLocal(long bytecodeIndex) {
    uint8_t bc1 = method->GetBytecode(bytecodeIndex + 1);
    uint8_t bc2 = method->GetBytecode(bytecodeIndex + 2);

    pVMObject local = _FRAME->GetLocal(bc1, bc2);

    _FRAME->Push(local);
}

void Interpreter::doPushArgument(long bytecodeIndex) {
    uint8_t bc1 = method->GetBytecode(bytecodeIndex + 1);
    uint8_t bc2 = method->GetBytecode(bytecodeIndex + 2);

    pVMObject argument = _FRAME->GetArgument(bc1, bc2);

    _FRAME->Push(argument);
}

void Interpreter::doPushField(long bytecodeIndex) {
    uint8_t fieldIndex = method->GetBytecode(bytecodeIndex + 1);
    pVMObject self = _SELF;
    pVMObject o;
#ifdef USE_TAGGING
    if (IS_TAGGED(self)) {
        o = GlobalBox::IntegerBox()->GetField(fieldIndex);
    }
    else {
        o = AS_POINTER(self)->GetField(fieldIndex);
    }
#else
    o = static_cast<VMObject*>(self)->GetField(fieldIndex);
#endif

    _FRAME->Push(o);
}

void Interpreter::doPushBlock(long bytecodeIndex) {
    // Short cut the negative case of #ifTrue: and #ifFalse:
    if (currentBytecodes[bytecodeIndexGlobal] == BC_SEND) {
        if (_FRAME->GetStackElement(0) == falseObject &&
            method->GetConstant(bytecodeIndexGlobal) == symbolIfTrue) {
            _FRAME->Push(nilObject);
            return;
        } else if (_FRAME->GetStackElement(0) == trueObject &&
                   method->GetConstant(bytecodeIndexGlobal) == symbolIfFalse) {
            _FRAME->Push(nilObject);
            return;
        }
    }

    pVMMethod blockMethod = static_cast<pVMMethod>(method->GetConstant(bytecodeIndex));

    long numOfArgs = blockMethod->GetNumberOfArguments();

    _FRAME->Push(_UNIVERSE->NewBlock(blockMethod, _FRAME, numOfArgs));
}

void Interpreter::doPushConstant(long bytecodeIndex) {
    pVMObject constant = method->GetConstant(bytecodeIndex);
    _FRAME->Push(constant);
}

void Interpreter::doPushGlobal(long bytecodeIndex) {
    pVMSymbol globalName = static_cast<pVMSymbol>(method->GetConstant(bytecodeIndex));
    pVMObject global = _UNIVERSE->GetGlobal(globalName);

    if(global != NULL)
        _FRAME->Push(global);
    else {
        pVMObject arguments[] = {globalName};
        pVMObject self = _SELF;

        //check if there is enough space on the stack for this unplanned Send
        //unknowGlobal: needs 2 slots, one for "this" and one for the argument
        long additionalStackSlots = 2 - _FRAME->RemainingStackSize();
        if (additionalStackSlots > 0) {
            _FRAME->SetBytecodeIndex(bytecodeIndexGlobal);
            //copy current frame into a bigger one and replace the current frame
            this->SetFrame(VMFrame::EmergencyFrameFrom(_FRAME,
                            additionalStackSlots));
        }

#ifdef USE_TAGGING
        if (IS_TAGGED(self))
            GlobalBox::IntegerBox()->Send(uG, arguments, 1);
        else
            AS_POINTER(self)->Send(uG, arguments, 1);
#else
        self->Send(uG, arguments, 1);
#endif
    }
}

void Interpreter::doPop() {
    _FRAME->Pop();
}

void Interpreter::doPopLocal(long bytecodeIndex) {
    uint8_t bc1 = method->GetBytecode(bytecodeIndex + 1);
    uint8_t bc2 = method->GetBytecode(bytecodeIndex + 2);

    pVMObject o = _FRAME->Pop();

    _FRAME->SetLocal(bc1, bc2, o);
}

void Interpreter::doPopArgument(long bytecodeIndex) {
    uint8_t bc1 = method->GetBytecode(bytecodeIndex + 1);
    uint8_t bc2 = method->GetBytecode(bytecodeIndex + 2);

    pVMObject o = _FRAME->Pop();
    _FRAME->SetArgument(bc1, bc2, o);
}

void Interpreter::doPopField(long bytecodeIndex) {
    uint8_t field_index = method->GetBytecode(bytecodeIndex + 1);

    pVMObject self = _SELF;
    pVMObject o = _FRAME->Pop();
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
    pVMSymbol signature = static_cast<pVMSymbol>(method->GetConstant(bytecodeIndex));

    long numOfArgs = Signature::GetNumberOfArguments(signature);

    pVMObject receiver = frame->GetStackElement(numOfArgs-1);
    assert(Universe::IsValidObject(receiver));
    assert(dynamic_cast<pVMClass>((pVMObject)receiver->GetClass()) != nullptr); // make sure it is really a class
    
#ifdef USE_TAGGING
    pVMClass receiverClass = IS_TAGGED(receiver) ? integerClass : AS_POINTER(receiver)->GetClass();
#else
    pVMClass receiverClass = receiver->GetClass();
#endif
    
    assert(Universe::IsValidObject(receiverClass));

#ifdef LOG_RECEIVER_TYPES
    _UNIVERSE->receiverTypes[receiverClass->GetName()->GetStdString()]++;
#endif

    this->send(signature, receiverClass);
}

void Interpreter::doSuperSend(long bytecodeIndex) {
    pVMSymbol signature = static_cast<pVMSymbol>(method->GetConstant(bytecodeIndex));

    pVMFrame ctxt = _FRAME->GetOuterContext();
    pVMMethod realMethod = ctxt->GetMethod();
    pVMClass holder = realMethod->GetHolder();
    pVMClass super = holder->GetSuperClass();
    pVMInvokable invokable = static_cast<pVMInvokable>(super->LookupInvokable(signature));

    if (invokable != NULL)
        (*invokable)(_FRAME);
    else {
        long numOfArgs = Signature::GetNumberOfArguments(signature);
        pVMObject receiver = _FRAME->GetStackElement(numOfArgs - 1);
        pVMArray argumentsArray = _UNIVERSE->NewArray(numOfArgs);

        for (long i = numOfArgs - 1; i >= 0; --i) {
            pVMObject o = _FRAME->Pop();
            argumentsArray->SetIndexableField(i, o);
        }
        pVMObject arguments[] = {signature, argumentsArray};
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
    pVMObject result = _FRAME->Pop();

    this->popFrameAndPushResult(result);
}

void Interpreter::doReturnNonLocal() {
    pVMObject result = _FRAME->Pop();

    pVMFrame context = _FRAME->GetOuterContext();

    if (!context->HasPreviousFrame()) {
        pVMBlock block = static_cast<pVMBlock>(_FRAME->GetArgument(0, 0));
        pVMFrame prevFrame = _FRAME->GetPreviousFrame();
        pVMFrame outerContext = prevFrame->GetOuterContext();
        pVMObject sender = outerContext->GetArgument(0, 0);
        pVMObject arguments[] = {block};

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
    pVMObject value = _FRAME->Pop();
    if (value == falseObject)
        doJump(bytecodeIndex);
}

void Interpreter::doJumpIfTrue(long bytecodeIndex) {
    pVMObject value = _FRAME->Pop();
    if (value == trueObject)
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

void Interpreter::WalkGlobals(VMOBJECT_PTR (*walk)(VMOBJECT_PTR)) {
    method = (pVMMethod) walk(method);
}

pVMThread Interpreter::GetThread(void) {
    return this->thread;
}

void Interpreter::SetThread(pVMThread thread) {
    this->thread = thread;
}

Page* Interpreter::GetPage() {
    return page;
}

void Interpreter::SetPage(Page* page) {
    this->page = page;
}

#if GC_TYPE==PAUSELESS
bool Interpreter::GetExpectedNMT() {
    return expectedNMT;
}

void Interpreter::FlipExpectedNMT() {
    expectedNMT = !expectedNMT;
}
#endif
