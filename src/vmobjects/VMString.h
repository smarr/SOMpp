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

class VMString: public AbstractVMObject {
public:
    typedef GCString Stored;
    
    VMString(const size_t length, const char* str);
    
    virtual int64_t GetHash() const {
        int64_t hash = 5381;

        for (size_t i = 0; i < length; i++) {
            hash = ((hash << 5) + hash) + chars[i];
        }
        
        return hash;
    }

    inline char* GetRawChars() const;
    StdString GetStdString() const;
    size_t GetStringLength() const;

    virtual VMString* Clone() const;
    virtual VMClass* GetClass() const;
    virtual size_t GetObjectSize() const;
    virtual void WalkObjects(walk_heap_fn);
    
    virtual void MarkObjectAsInvalid();
    
    virtual StdString AsDebugString() const;

protected_testable:
    //this could be replaced by the CHARS macro in VMString.cpp
    //in order to decrease the object size
    const size_t length;
    char* const chars;
protected:
    VMString(char* const str, const size_t length) : chars(str), length(length) {}; //constructor to use by VMSymbol
};

char* VMString::GetRawChars() const {
    return chars;
}
