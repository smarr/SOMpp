#pragma once

#ifndef CORE_INTEGER_H_
#define CORE_INTEGER_H_

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
 
class _Integer : public PrimitiveContainer
{

public:

    void  Plus(pVMObject object, pVMFrame frame);
    void  Minus(pVMObject object, pVMFrame frame);
    void  Star(pVMObject object, pVMFrame frame);
    void  Slash(pVMObject object, pVMFrame frame);
    void  Slashslash(pVMObject object, pVMFrame frame);
    void  Percent(pVMObject object, pVMFrame frame);
    void  And(pVMObject object, pVMFrame frame);
    void  Equal(pVMObject object, pVMFrame frame);
    void  Lowerthan(pVMObject object, pVMFrame frame);
    void  AsString(pVMObject object, pVMFrame frame);
    void  Sqrt(pVMObject object, pVMFrame frame);
    void  AtRandom(pVMObject object, pVMFrame frame);

    _Integer(void);

private:
    void pushResult(pVMObject object, pVMFrame frame, int64_t result);
    void resendAsBigInteger(pVMObject object, 
                                  const char* op,
                                  pVMInteger left, pVMBigInteger right);
    void resendAsDouble(pVMObject object, const char* op,
                    pVMInteger left, pVMDouble right);

};


#endif
