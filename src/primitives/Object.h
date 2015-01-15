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

class VMObject;
class VMFrame;
#include "../primitivesCore/PrimitiveContainer.h"

class _Object: public PrimitiveContainer {
public:
    _Object();
    void Equalequal(VMObject* object, VMFrame* frame);
    void ObjectSize(VMObject* object, VMFrame* frame);
    void Hashcode(VMObject* object, VMFrame* frame);
    void Inspect(VMObject* object, VMFrame* frame);
    void Halt(VMObject* object, VMFrame* frame);

    void Perform(VMObject* object, VMFrame* frame);
    void PerformWithArguments(VMObject* object, VMFrame* frame);
    void PerformInSuperclass(VMObject* object, VMFrame* frame);
    void PerformWithArgumentsInSuperclass(VMObject* object, VMFrame* frame);

    void InstVarAt(VMObject* object, VMFrame* frame);
    void InstVarAtPut(VMObject* object, VMFrame* frame);
    void InstVarNamed(VMObject* object, VMFrame* frame);
    
    void Class(VMObject* object, VMFrame* frame);
};
