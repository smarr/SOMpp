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

extern pVMClass stringClass;

//this macro could replace the chars member variable
//#define CHARS ((char*)&clazz+sizeof(pVMObject))

VMString::VMString(const char* str) : AbstractVMObject() {
    //set the chars-pointer to point at the position of the first character
    chars = (char*) &chars + sizeof(char*);

    size_t i = 0;
    size_t len = strlen(str);
    for (; i < len; ++i) {
        chars[i] = str[i];
    }
    chars[i] = '\0';

}

pVMString VMString::Clone() const {
#if GC_TYPE==GENERATIONAL
    return new (GetHeap<HEAP_CLS>(), PADDED_SIZE(strlen(chars)+1), true) VMString(chars);
#else
    return new (GetHeap<HEAP_CLS>(), PADDED_SIZE(strlen(chars)+1)) VMString(chars);
#endif
}

void VMString::MarkObjectAsInvalid() {
    size_t i = 0;
    while (chars[i] != '\0') {
        chars[i] = 'z';
        i++;
    }
}

void VMString::WalkObjects(VMOBJECT_PTR (VMOBJECT_PTR)) {
    //nothing to do
}

VMString::VMString(const StdString& s) {
    VMString(s.c_str());
}

size_t VMString::GetObjectSize() const {
    size_t size = sizeof(VMString) + PADDED_SIZE(strlen(chars) + 1);
    return size;
}

pVMClass VMString::GetClass() const {
    return stringClass;
}

size_t VMString::GetStringLength() const {
    //get the additional memory allocated by this object and substract one
    //for the '0' character and four for the char*
    return strlen(chars);
}

StdString VMString::GetStdString() const {
    if (chars == 0)
        return StdString("");
    return StdString(chars);
}
