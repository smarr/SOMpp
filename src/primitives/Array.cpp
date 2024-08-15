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

#include "Array.h"

#include <cstdint>

#include "../primitivesCore/PrimitiveContainer.h"
#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMArray.h"
#include "../vmobjects/VMFrame.h"

static vm_oop_t arrAt(vm_oop_t leftObj, vm_oop_t idx) {
    auto* self = static_cast<VMArray*>(leftObj);
    return self->GetIndexableField(INT_VAL(idx) - 1);
}

static vm_oop_t arrAtPut(vm_oop_t rcvr, vm_oop_t index, vm_oop_t value) {
    auto* self = static_cast<VMArray*>(rcvr);
    int64_t const i = INT_VAL(index);
    self->SetIndexableField(i - 1, value);
    return rcvr;
}

static vm_oop_t arrLength(vm_oop_t leftObj) {
    auto* self = static_cast<VMArray*>(leftObj);
    return NEW_INT((int64_t)self->GetNumberOfIndexableFields());
}

static vm_oop_t arrNew(vm_oop_t /*unused*/, vm_oop_t arg) {
    int64_t const size = INT_VAL(arg);
    return Universe::NewArray(size);
}

static vm_oop_t arrCopy(vm_oop_t rcvr) {
    auto* self = static_cast<VMArray*>(rcvr);
    return self->Copy();
}

_Array::_Array() : PrimitiveContainer() {
    Add("new:", &arrNew, true);
    Add("at:", &arrAt, false);
    Add("at:put:", &arrAtPut, false);
    Add("length", &arrLength, false);
    Add("copy", &arrCopy, false);
}
