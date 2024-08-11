#pragma once

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

#include "AbstractObject.h"

class VMString : public AbstractVMObject {
public:
    typedef GCString Stored;

    VMString(const size_t length, const char* str)
        : AbstractVMObject(), length(length),
          // set the chars-pointer to point at the position of the first
          // character
          chars((char*)&chars + sizeof(char*)) {
        for (size_t i = 0; i < length; i++) {
            chars[i] = str[i];
        }
    }

    int64_t GetHash() const override {
        uint64_t hash = 5381U;

        for (size_t i = 0; i < length; i++) {
            hash = ((hash << 5U) + hash) + chars[i];
        }

        return (int64_t)hash;
    }

    inline char* GetRawChars() const;
    StdString GetStdString() const;
    size_t GetStringLength() const;

    VMString* CloneForMovingGC() const override;
    VMClass* GetClass() const override;
    size_t GetObjectSize() const override;
    void WalkObjects(walk_heap_fn) override;

    void MarkObjectAsInvalid() override;
    bool IsMarkedInvalid() const override;

    StdString AsDebugString() const override;

    make_testable(public);

    // this could be replaced by the CHARS macro in VMString.cpp
    // in order to decrease the object size
    const size_t length;
    char* chars;

protected:
    VMString(char* const adaptedCharsPointer, const size_t length)
        :  // set the chars-pointer to point at the position of the first
           // character as determined in the VMSymbol constructor
          length(length),
          chars(adaptedCharsPointer) {};  // constructor to use by VMSymbol
};

char* VMString::GetRawChars() const {
    return chars;
}
