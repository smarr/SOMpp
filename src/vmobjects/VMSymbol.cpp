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

#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>

#include "../memory/Heap.h"
#include "../misc/defs.h"
#include "../vm/Globals.h"  // NOLINT (misc-include-cleaner)
#include "AbstractObject.h"
#include "ObjectFormats.h"
#include "Signature.h"
#include "VMClass.h"
#include "VMInvokable.h"
#include "VMString.h"
#include "VMSymbol.h"

VMSymbol::VMSymbol(const size_t length, const char* const str) :
      // set the chars-pointer to point at the position of the first character
      VMString((char*) ((intptr_t)&cachedInvokable + (3 * sizeof(VMInvokable*))), length),
      numberOfArgumentsOfSignature(Signature::DetermineNumberOfArguments(str, length)) {
    nextCachePos = 0;
    size_t i = 0;
    for (; i < length; ++i) {
        chars[i] = str[i];
    }
    //clear caching fields
    memset(&cachedClass_invokable, 0, 6 * sizeof(void*) + 1 * sizeof(long));
}

size_t VMSymbol::GetObjectSize() const {
    size_t size = sizeof(VMSymbol) + PADDED_SIZE(length);
    return size;
}

VMSymbol* VMSymbol::Clone() const {
    VMSymbol* result = new (GetHeap<HEAP_CLS>(), PADDED_SIZE(length) ALLOC_MATURE) VMSymbol(length, chars);
    return result;
}

VMClass* VMSymbol::GetClass() const {
    return load_ptr(symbolClass);
}

std::string VMSymbol::GetPlainString() const {
    ostringstream str;
    char* chars = this->chars;
    size_t length = this->length;
    for (size_t i = 0; i < length; i++) {
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
    std::string st = str.str();

    return st;
}

void VMSymbol::WalkObjects(walk_heap_fn walk) {
    for (long i = 0; i < nextCachePos; i++) {
        cachedClass_invokable[i] = static_cast<GCClass*>(walk(
                                        const_cast<GCClass*>(cachedClass_invokable[i])));
        cachedInvokable[i] = static_cast<GCInvokable*>(walk(cachedInvokable[i]));
    }
}

std::string VMSymbol::AsDebugString() const {
    return "Symbol(" + GetStdString() + ")";
}

VMInvokable* VMSymbol::GetCachedInvokable(const VMClass* cls) const {
    if (cls == load_ptr(cachedClass_invokable[0]))
        return load_ptr(cachedInvokable[0]);
    else if (cls == load_ptr(cachedClass_invokable[1]))
        return load_ptr(cachedInvokable[1]);
    else if (cls == load_ptr(cachedClass_invokable[2]))
        return load_ptr(cachedInvokable[2]);
    return nullptr;
}

void VMSymbol::UpdateCachedInvokable(const VMClass* cls, VMInvokable* invo) {
    store_ptr(cachedInvokable[nextCachePos], invo);
    store_ptr(cachedClass_invokable[nextCachePos], const_cast<VMClass*>(cls));
    nextCachePos = (nextCachePos + 1) % 3;
}
