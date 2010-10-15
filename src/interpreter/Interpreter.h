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

class VMMethod;
class VMFrame;
class VMMethod;
class VMObject;
class VMSymbol;
class VMClass;

class Interpreter {
public:
    Interpreter();
    ~Interpreter();
    void Start();
    pVMFrame PushNewFrame(pVMMethod method);
    void SetFrame(pVMFrame frame);
    pVMFrame GetFrame();
    pVMMethod GetMethod();
    pVMObject GetSelf();
private:
    pVMFrame frame;
    StdString uG;
    StdString dnu;
    StdString eB;

    pVMFrame popFrame();
    void popFrameAndPushResult(pVMObject result);
    void send(pVMSymbol signature, pVMClass receiverClass);
    
    void doDup();
    void doPushLocal(int bytecodeIndex);
    void doPushArgument(int bytecodeIndex);
    void doPushField(int bytecodeIndex);
    void doPushBlock(int bytecodeIndex);
    void doPushConstant(int bytecodeIndex);
    void doPushGlobal(int bytecodeIndex);
    void doPop(void);
    void doPopLocal(int bytecodeIndex);
    void doPopArgument(int bytecodeIndex);
    void doPopField(int bytecodeIndex);
    void doSend(int bytecodeIndex);
    void doSuperSend(int bytecodeIndex);
    void doReturnLocal();
    void doReturnNonLocal();
};

#endif
