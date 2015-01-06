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

#include "../vmobjects/VMObject.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMSymbol.h"

#include "../vm/Universe.h"
#include "../primitivesCore/Routine.h"
#include "Symbol.h"

void _Symbol::AsString(Interpreter*, VMFrame* frame) {
    VMSymbol* sym = static_cast<VMSymbol*>(frame->Pop());

    StdString str = sym->GetStdString();
    frame->Push(GetUniverse()->NewString(str));
}

void _Symbol::Equal(Interpreter*, VMFrame* frame) {
    vm_oop_t op1 = frame->Pop();
    vm_oop_t op2 = frame->Pop();
    
    if (op1 == op2) {
        frame->Push(load_ptr(trueObject));
    } else {
        frame->Push(load_ptr(falseObject));
    }
}

_Symbol::_Symbol() : PrimitiveContainer() {
    SetPrimitive("asString", new Routine<_Symbol>(this, &_Symbol::AsString, false));
    SetPrimitive("equal",    new Routine<_Symbol>(this, &_Symbol::Equal,    false));
}
