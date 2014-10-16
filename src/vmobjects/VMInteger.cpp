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

#include "VMInteger.h"
#include "../vm/Universe.h"
#include "../interpreter/Interpreter.h"

VMInteger::VMInteger() {
    embeddedInteger = 0;
}

VMInteger::VMInteger(long val) {
    embeddedInteger = val;
}

#if GC_TYPE==GENERATIONAL
pVMInteger VMInteger::Clone() {
    return new (_HEAP, _PAGE, 0, true) VMInteger(*this);
}
#elif GC_TYPE==PAUSELESS
pVMInteger VMInteger::Clone(Page* page) {
    return new (page) VMInteger(*this);
}
#else
pVMInteger VMInteger::Clone() {
    return new (_HEAP) VMInteger(*this);
}
#endif

pVMClass VMInteger::GetClass() {
    return READBARRIER(integerClass);
}
