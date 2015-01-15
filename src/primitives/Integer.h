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
class VMInteger;
class VMBigInteger;
class VMDouble;

#include "../primitivesCore/PrimitiveContainer.h"

class _Integer: public PrimitiveContainer {

public:

    void Plus(VMObject* object, VMFrame* frame);
    void Minus(VMObject* object, VMFrame* frame);
    void Star(VMObject* object, VMFrame* frame);
    void BitwiseAnd(VMObject* object, VMFrame* frame);
    void Slash(VMObject* object, VMFrame* frame);
    void Slashslash(VMObject* object, VMFrame* frame);
    void Percent(VMObject* object, VMFrame* frame);
    void And(VMObject* object, VMFrame* frame);
    void Equal(VMObject* object, VMFrame* frame);
    void Lowerthan(VMObject* object, VMFrame* frame);
    void AsString(VMObject* object, VMFrame* frame);
    void Sqrt(VMObject* object, VMFrame* frame);
    void AtRandom(VMObject* object, VMFrame* frame);

    void FromString(VMObject* object, VMFrame* frame);

    _Integer(void);

private:
    void pushResult(VMObject* object, VMFrame* frame, int64_t result);
    void resendAsBigInteger(VMObject* object, const char* op, VMInteger* left,
            pVMBigInteger right);
    void resendAsDouble(VMObject* object, const char* op, VMInteger* left,
            VMDouble* right);

};
