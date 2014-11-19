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

#include "VMBigInteger.h"

#include <vmobjects/AbstractObject.h>
#include <vmobjects/VMClass.h>

#include <vm/Universe.h>
#include "../interpreter/Interpreter.h"

VMBigInteger::VMBigInteger() {
    this->embeddedInteger = 0;
}

VMBigInteger::VMBigInteger(int64_t val) {
    this->embeddedInteger = val;
}

#if GC_TYPE==GENERATIONAL
pVMBigInteger VMBigInteger::Clone() {
    return new (_HEAP, _PAGE, 0, true) VMBigInteger(*this);
}
#elif GC_TYPE==PAUSELESS
pVMBigInteger VMBigInteger::Clone(Interpreter* thread) {
    pVMBigInteger clone = new (_HEAP, thread) VMBigInteger(*this);
    clone->IncreaseVersion();
    this->MarkObjectAsInvalid();
    return clone;
}
pVMBigInteger VMBigInteger::Clone(PauselessCollectorThread* thread) {
    pVMBigInteger clone = new (_HEAP, thread) VMBigInteger(*this);
    clone->IncreaseVersion();
    this->MarkObjectAsInvalid();
    return clone;
}
#else
pVMBigInteger VMBigInteger::Clone() {
    return new (_HEAP) VMBigInteger(*this);
}
#endif

size_t VMBigInteger::GetObjectSize() const {
    return sizeof(VMBigInteger);
}

pVMClass VMBigInteger::GetClass() {
    return READBARRIER(bigIntegerClass);
}
