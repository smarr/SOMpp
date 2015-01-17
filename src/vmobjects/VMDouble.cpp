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

#include "VMDouble.h"

#include <vm/Universe.h>
#include "../interpreter/Interpreter.h"

#if GC_TYPE==GENERATIONAL
VMDouble* VMDouble::Clone() {
    return new (_HEAP, _PAGE, 0, true) VMDouble(*this);
}
#elif GC_TYPE==PAUSELESS
VMDouble* VMDouble::Clone(Interpreter* thread) {
    VMDouble* clone = new (_HEAP, thread) VMDouble(*this);
    /* clone->IncreaseVersion();
    this->MarkObjectAsInvalid(); */
    return clone;
}
VMDouble* VMDouble::Clone(PauselessCollectorThread* thread) {
    VMDouble* clone = new (_HEAP, thread) VMDouble(*this);
    /* clone->IncreaseVersion();
    this->MarkObjectAsInvalid(); */
    return clone;
}
#else
VMDouble* VMDouble::Clone() {
    return new (_HEAP) VMDouble(*this);
}
#endif

VMClass* VMDouble::GetClass() {
    return load_ptr(doubleClass);
}

StdString VMDouble::AsDebugString() {
    return "Double(" + to_string(embeddedDouble) + ")";
}
