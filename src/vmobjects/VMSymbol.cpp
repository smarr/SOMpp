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

#include <sstream>
#include <string.h>

#include "VMSymbol.h"
#include "VMInteger.h"
#include "Signature.h"

extern GCClass* symbolClass;

VMSymbol::VMSymbol(const char* str) :
  numberOfArgumentsOfSignature(Signature::DetermineNumberOfArguments(str)) {
    nextCachePos = 0;
    // set the chars-pointer to point at the position of the first character
    chars = (char*) &cachedInvokable + +3 * sizeof(VMInvokable*);
    size_t i = 0;
    for (; i < strlen(str); ++i) {
        chars[i] = str[i];
    }
    chars[i] = '\0';
    //clear caching fields
    memset(&cachedClass_invokable, 0, 6 * sizeof(void*) + 1 * sizeof(long));
}

VMSymbol::VMSymbol(const StdString& s) :
  numberOfArgumentsOfSignature(Signature::DetermineNumberOfArguments(s.c_str())) {
    VMSymbol(s.c_str());
}

size_t VMSymbol::GetObjectSize() const {
    size_t size = sizeof(VMSymbol) + PADDED_SIZE(strlen(chars) + 1);
    return size;
}

VMSymbol* VMSymbol::Clone() const {
    VMSymbol* result = new (GetHeap<HEAP_CLS>(), PADDED_SIZE(strlen(chars) + 1) ALLOC_MATURE) VMSymbol(chars);
    return result;
}

VMClass* VMSymbol::GetClass() const {
    return load_ptr(symbolClass);
}

StdString VMSymbol::GetPlainString() const {
    ostringstream str;
    char* chars = GetChars();
    size_t length = GetStringLength();
    for (size_t i = 0; i <= length; i++) {
        char c = chars[i];
        switch (c) {
        case '~':
            str << "tilde";
            break;
        case '&':
            str << "and";
            break;
        case '|':
            str << "bar";
            break;
        case '*':
            str << "star";
            break;
        case '/':
            str << "slash";
            break;
        case '@':
            str << "at";
            break;
        case '+':
            str << "plus";
            break;
        case '-':
            str << "minus";
            break;
        case '=':
            str << "equal";
            break;
        case '>':
            str << "greaterthan";
            break;
        case '<':
            str << "lowerthan";
            break;
        case ',':
            str << "comma";
            break;
        case '%':
            str << "percent";
            break;
        case '\\':
            str << "backslash";
            break;
        case ':':
            str << '_';
            break;
        default:
            if (c != 0) {
                str << c;
            }
            break;
        }
    }
    StdString st = str.str();

    return st;
}

void VMSymbol::WalkObjects(walk_heap_fn walk) {
    for (long i = 0; i < nextCachePos; i++) {
        cachedClass_invokable[i] = static_cast<GCClass*>(walk(
                                        const_cast<GCClass*>(cachedClass_invokable[i])));
        cachedInvokable[i] = static_cast<GCInvokable*>(walk(cachedInvokable[i]));
    }
}

StdString VMSymbol::AsDebugString() const {
    return "Symbol(" + GetStdString() + ")";
}

