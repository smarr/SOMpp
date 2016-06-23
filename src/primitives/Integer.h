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

#include <vmobjects/ObjectFormats.h>
#include <primitivesCore/PrimitiveContainer.h>

class _Integer: public PrimitiveContainer {

public:

    void Plus(Interpreter*, VMFrame*);
    void Minus(Interpreter*, VMFrame*);
    void Star(Interpreter*, VMFrame*);
    void Rem(Interpreter*, VMFrame*);
    void BitwiseAnd(Interpreter*, VMFrame*);
    void BitwiseXor(Interpreter*, VMFrame*);
    void LeftShift(Interpreter*, VMFrame*);
    void UnsignedRightShift(Interpreter*, VMFrame*);
    void Slash(Interpreter*, VMFrame*);
    void Slashslash(Interpreter*, VMFrame*);
    void Percent(Interpreter*, VMFrame*);
    void And(Interpreter*, VMFrame*);
    void Equal(Interpreter*, VMFrame*);
    void EqualEqual(Interpreter*, VMFrame*);
    void Lowerthan(Interpreter*, VMFrame*);
    void AsString(Interpreter*, VMFrame*);
    void As32BitSigned(Interpreter*, VMFrame*);
    void As32BitUnsigned(Interpreter*, VMFrame*);
    void Sqrt(Interpreter*, VMFrame*);
    void AtRandom(Interpreter*, VMFrame*);

    void FromString(Interpreter*, VMFrame*);

    _Integer(void);

private:

    void resendAsDouble(Interpreter* interp, const char* op, vm_oop_t left, VMDouble* right);

};
