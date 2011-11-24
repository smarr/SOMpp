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

#define protected public
#include "Interpreter.h"
#include "bytecodes.h"

#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMObject.h"
#include "../vmobjects/VMSymbol.h"
#include "../vmobjects/VMInvokable.h"
#include "../vmobjects/Signature.h"
#ifdef USE_TAGGING
#include "../vmobjects/VMPointerConverter.h"
#endif

#include "../compiler/Disassembler.h"


// convenience macros for frequently used function invocations
#define _FRAME this->GetFrame()
#define _SETFRAME(f) this->SetFrame(f)
#define _METHOD this->GetMethod()
#define _SELF this->GetSelf()


Interpreter::Interpreter() {
    this->frame = NULL;
    
    uG = "unknownGlobal:";
    dnu = "doesNotUnderstand:arguments:";
    eB = "escapedBlock:";
    
}


Interpreter::~Interpreter() {
    
}


// The following three variables are needed for caching
int32_t bytecodeIndex;
int32_t nextBytecodeIndex;
pVMMethod method = NULL;

void Interpreter::Start() {
  method = _FRAME->GetMethod();
  while (true) {
    //we reached a safe collection point
    // perform collection if triggered
    if (_HEAP->isCollectionTriggered()) {
      _FRAME->SetBytecodeIndex(nextBytecodeIndex);
      _HEAP->FullGC();
      method = _FRAME->GetMethod();
    }

#ifndef CACHE_BCINDEX
    bytecodeIndex = _FRAME->GetBytecodeIndex();
#endif
    uint8_t bytecode = method->GetBytecode(bytecodeIndex);
    int bytecodeLength = Bytecode::GetBytecodeLength(bytecode);
    if(dumpBytecodes >1)
      Disassembler::DumpBytecode(_FRAME, method, bytecodeIndex);
    nextBytecodeIndex = bytecodeIndex + bytecodeLength;

#ifndef CACHE_BCINDEX
    _FRAME->SetBytecodeIndex(nextBytecodeIndex);
#endif

    // Handle the current bytecode
    switch(bytecode) {
      case BC_HALT:             return; // handle the halt bytecode
      case BC_DUP:              doDup(); break;
      case BC_PUSH_LOCAL:       doPushLocal(bytecodeIndex); break;
      case BC_PUSH_ARGUMENT:    doPushArgument(bytecodeIndex); break;
      case BC_PUSH_FIELD:       doPushField(bytecodeIndex); break;
      case BC_PUSH_BLOCK:       doPushBlock(bytecodeIndex); break;
      case BC_PUSH_CONSTANT:    doPushConstant(bytecodeIndex); break;
      case BC_PUSH_GLOBAL:      doPushGlobal(bytecodeIndex); break;
      case BC_POP:              doPop(); break;
      case BC_POP_LOCAL:        doPopLocal(bytecodeIndex); break;
      case BC_POP_ARGUMENT:     doPopArgument(bytecodeIndex); break;
      case BC_POP_FIELD:        doPopField(bytecodeIndex); break;
#ifdef CACHE_BCINDEX
      case BC_SEND:             _FRAME->SetBytecodeIndex(nextBytecodeIndex);
                                doSend(bytecodeIndex);
                                nextBytecodeIndex = _FRAME->GetBytecodeIndex();
                                break;
      case BC_SUPER_SEND:       _FRAME->SetBytecodeIndex(nextBytecodeIndex);
                                doSuperSend(bytecodeIndex);
                                nextBytecodeIndex = _FRAME->GetBytecodeIndex();
                                break;
#else
      case BC_SEND:             doSend(bytecodeIndex); break;
      case BC_SUPER_SEND:       doSuperSend(bytecodeIndex); break;
#endif
      case BC_RETURN_LOCAL:     doReturnLocal(); break;
      case BC_RETURN_NON_LOCAL: doReturnNonLocal(); break;
      default:                  _UNIVERSE->ErrorExit(
                                    "Interpreter: Unexpected bytecode"); 
    } // switch
#ifdef CACHE_BCINDEX
    bytecodeIndex = nextBytecodeIndex;
#endif
  } // while
}


pVMFrame Interpreter::PushNewFrame( pVMMethod method ) {
    _SETFRAME(_UNIVERSE->NewFrame(_FRAME, method));
    return _FRAME;
}


void Interpreter::SetFrame( pVMFrame frame ) {
#ifdef CACHE_BCINDEX
  if (this->frame != NULL)
    this->frame->SetBytecodeIndex(nextBytecodeIndex);
#endif
  this->frame = frame;   
#ifdef CACHE_BCINDEX
  nextBytecodeIndex = frame->GetBytecodeIndex();
#endif
  method = frame->GetMethod();
}


pVMFrame Interpreter::GetFrame() {
    return this->frame;
}


pVMMethod Interpreter::GetMethod() {
    return _FRAME->GetMethod();
}


pVMObject Interpreter::GetSelf() {
    pVMFrame context = _FRAME->GetOuterContext();
    return context->GetArgument(0,0);
}


pVMFrame Interpreter::popFrame() {
    pVMFrame result = _FRAME;
    this->SetFrame(_FRAME->GetPreviousFrame());

    result->ClearPreviousFrame();

    return result;
}


void Interpreter::popFrameAndPushResult( pVMObject result ) {
    pVMFrame prevFrame = this->popFrame();

    pVMMethod method = prevFrame->GetMethod();
    int numberOfArgs = method->GetNumberOfArguments();

    for (int i = 0; i < numberOfArgs; ++i) _FRAME->Pop();

    _FRAME->Push(result);
}


void Interpreter::send( pVMSymbol signature, pVMClass receiverClass) {
  pVMInvokable invokable =
#ifdef USE_TAGGING
      DynamicConvert<VMInvokable, AbstractVMObject>( receiverClass->LookupInvokable(signature) );
#else
  dynamic_cast<pVMInvokable>( receiverClass->LookupInvokable(signature) );
#endif

  if (invokable != NULL) {
#ifdef LOG_RECEIVER_TYPES
    StdString name = receiverClass->GetName()->GetStdString();
    if (_UNIVERSE->callStats.find(name) == _UNIVERSE->callStats.end())
      _UNIVERSE->callStats[name] = {0,0};
    _UNIVERSE->callStats[name].noCalls++;
    if (invokable->IsPrimitive())
      _UNIVERSE->callStats[name].noPrimitiveCalls++;
#endif

    (*invokable)(_FRAME);
  } else {
    //doesNotUnderstand
    int numberOfArgs = Signature::GetNumberOfArguments(signature);

    pVMObject receiver = _FRAME->GetStackElement(numberOfArgs-1);

    pVMArray argumentsArray = _UNIVERSE->NewArray(numberOfArgs);

    for (int i = numberOfArgs - 1; i >= 0; --i) {
      pVMObject o = _FRAME->Pop();
      argumentsArray->SetIndexableField(i, o);
    }
    pVMObject arguments[] = { (pVMObject)signature, 
      (pVMObject)argumentsArray };

    //check if current frame is big enough for this unplanned Send
    //doesNotUnderstand: needs 3 slots, one for this, one for method name, one for args
    int additionalStackSlots = 3 - _FRAME->RemainingStackSize();
    if (additionalStackSlots > 0) {
      //before creating the emergency frame, we have to update the  current frame's bcIndex which is cached
      _FRAME->SetBytecodeIndex(nextBytecodeIndex);
      //copy current frame into a bigger one and replace the current frame
      this->SetFrame(VMFrame::EmergencyFrameFrom(_FRAME, additionalStackSlots));
    }

    receiver->Send(dnu, arguments, 2);
  }
}


void Interpreter::doDup() {
    pVMObject elem = _FRAME->GetStackElement(0);
    _FRAME->Push(elem);
}


void Interpreter::doPushLocal( int bytecodeIndex ) {
    uint8_t bc1 = method->GetBytecode(bytecodeIndex + 1);
    uint8_t bc2 = method->GetBytecode(bytecodeIndex + 2);

    pVMObject local = _FRAME->GetLocal(bc1, bc2);

    _FRAME->Push(local);
}


void Interpreter::doPushArgument( int bytecodeIndex ) {
    uint8_t bc1 = method->GetBytecode(bytecodeIndex + 1);
    uint8_t bc2 = method->GetBytecode(bytecodeIndex + 2);

    pVMObject argument = _FRAME->GetArgument(bc1, bc2);

    _FRAME->Push(argument);
}


void Interpreter::doPushField( int bytecodeIndex ) {
    pVMSymbol fieldName = (pVMSymbol) method->GetConstant(bytecodeIndex);

    pVMObject self = _SELF;
    int fieldIndex = self->GetFieldIndex(fieldName);

    pVMObject o = self->GetField(fieldIndex);

    _FRAME->Push(o);
}


void Interpreter::doPushBlock( int bytecodeIndex ) {
    pVMMethod blockMethod = (pVMMethod)(method->GetConstant(bytecodeIndex));

    int numOfArgs = blockMethod->GetNumberOfArguments();

    _FRAME->Push((pVMObject) _UNIVERSE->NewBlock(blockMethod, _FRAME,
                                                 numOfArgs));
}


void Interpreter::doPushConstant( int bytecodeIndex ) {
    pVMObject constant = method->GetConstant(bytecodeIndex);
    _FRAME->Push(constant);
}


void Interpreter::doPushGlobal( int bytecodeIndex) {
  pVMSymbol globalName = (pVMSymbol) method->GetConstant(bytecodeIndex);
  pVMObject global = _UNIVERSE->GetGlobal(globalName);

  if(global != NULL)
    _FRAME->Push(global);
  else {
    pVMObject arguments[] = { (pVMObject) globalName };
    pVMObject self = _SELF;

    //check if there is enough space on the stack for this unplanned Send
    //unknowGlobal: needs 2 slots, one for "this" and one for the argument
    int additionalStackSlots = 2 - _FRAME->RemainingStackSize();       
    if (additionalStackSlots > 0) {
      //before creating the emergency frame, we have to update the  current frame's bcIndex which is cached
      _FRAME->SetBytecodeIndex(nextBytecodeIndex);
      //copy current frame into a bigger one and replace the current frame
      this->SetFrame(VMFrame::EmergencyFrameFrom(_FRAME,
                                                 additionalStackSlots));
    }
    self->Send(uG, arguments, 1);
  }
}


void Interpreter::doPop() {
    _FRAME->Pop();
}


void Interpreter::doPopLocal( int bytecodeIndex ) {
    uint8_t bc1 = method->GetBytecode(bytecodeIndex + 1);
    uint8_t bc2 = method->GetBytecode(bytecodeIndex + 2);

    pVMObject o = _FRAME->Pop();

    _FRAME->SetLocal(bc1, bc2, o);
}


void Interpreter::doPopArgument( int bytecodeIndex ) {
    uint8_t bc1 = method->GetBytecode(bytecodeIndex + 1);
    uint8_t bc2 = method->GetBytecode(bytecodeIndex + 2);

    pVMObject o = _FRAME->Pop();
    _FRAME->SetArgument(bc1, bc2, o);
}


void Interpreter::doPopField( int bytecodeIndex ) {
    pVMSymbol field_name = (pVMSymbol) method->GetConstant(bytecodeIndex);

    pVMObject self = _SELF;
    int field_index = self->GetFieldIndex(field_name);

    pVMObject o = _FRAME->Pop();
    self->SetField(field_index, o);
}


void Interpreter::doSend( int bytecodeIndex ) {
    pVMSymbol signature = (pVMSymbol) method->GetConstant(bytecodeIndex);

    int numOfArgs = Signature::GetNumberOfArguments(signature);

    pVMObject receiver = _FRAME->GetStackElement(numOfArgs-1);

#ifdef LOG_RECEIVER_TYPES
    _UNIVERSE->receiverTypes[receiver->GetClass()->GetName()->GetStdString()]++;
#endif

    this->send(signature, receiver->GetClass());
}


void Interpreter::doSuperSend( int bytecodeIndex ) {
    pVMSymbol signature = (pVMSymbol) method->GetConstant(bytecodeIndex);

    pVMFrame ctxt = _FRAME->GetOuterContext();
    pVMMethod realMethod = ctxt->GetMethod();
    pVMClass holder = realMethod->GetHolder();
    pVMClass super = holder->GetSuperClass();
#ifdef USE_TAGGING
    pVMInvokable invokable = DynamicConvert<VMInvokable, VMObject>(
                                    super->LookupInvokable(signature) );
#else
    pVMInvokable invokable = dynamic_cast<pVMInvokable>( super->LookupInvokable(signature) );
#endif

    if (invokable != NULL)
        (*invokable)(_FRAME);
    else {
        int numOfArgs = Signature::GetNumberOfArguments(signature);
        pVMObject receiver = _FRAME->GetStackElement(numOfArgs - 1);
        pVMArray argumentsArray = _UNIVERSE->NewArray(numOfArgs);

        for (int i = numOfArgs - 1; i >= 0; --i) {
            pVMObject o = _FRAME->Pop();
            argumentsArray->SetIndexableField(i, o);
        }
        pVMObject arguments[] = { (pVMObject)signature, 
                                  (pVMObject) argumentsArray };
        receiver->Send(dnu, arguments, 2);
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
        pVMBlock block = (pVMBlock) _FRAME->GetArgument(0, 0);
        pVMFrame prevFrame = _FRAME->GetPreviousFrame();
        pVMFrame outerContext = prevFrame->GetOuterContext();
        pVMObject sender = outerContext->GetArgument(0, 0);
        pVMObject arguments[] = { (pVMObject)block };

        this->popFrame();

        sender->Send(eB, arguments, 1);
        return;
    }

    while (_FRAME != context) this->popFrame();

    this->popFrameAndPushResult(result);
}

