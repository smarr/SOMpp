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

#include <string.h>
#include <iostream>

#include "VMString.h"

extern GCClass* stringClass;

//this macro could replace the chars member variable
//#define CHARS ((char*)&clazz+sizeof(VMObject*))

VMString::VMString(const size_t length, const char* str) :
        length(length),
        // set the chars-pointer to point at the position of the first character
        chars((char*) &chars + sizeof(char*)),
        AbstractVMObject() {
    for (size_t i = 0; i < length; i++) {
        chars[i] = str[i];
    }
}

VMString* VMString::Clone() const {
    return new (GetHeap<HEAP_CLS>(), PADDED_SIZE(length) ALLOC_MATURE) VMString(length, chars);
}

void VMString::MarkObjectAsInvalid() {
    for (size_t i = 0; i < length; i++) {
        chars[i] = 'z';
        i++;
    }
}

void VMString::WalkObjects(walk_heap_fn) {
    //nothing to do
}

size_t VMString::GetObjectSize() const {
    size_t size = sizeof(VMString) + PADDED_SIZE(length);
    return size;
}

VMClass* VMString::GetClass() const {
    return load_ptr(stringClass);
}

size_t VMString::GetStringLength() const {
    return length;
}

StdString VMString::GetStdString() const {
    if (chars == 0)
        return StdString("");
    return StdString(chars, length);
}

StdString VMString::AsDebugString() const {
    return "String(" + GetStdString() + ")";
}
