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

#include "VMObject.h"

class VMBlock: public VMObject {
public:
    typedef GCBlock Stored;
    
    VMBlock();

            VMMethod* GetMethod() const;
            void      SetMethod(VMMethod*);
    inline  void      SetContext(VMFrame*);
    inline  VMFrame*  GetContext() const;
    virtual VMBlock*  Clone() const;
    
    virtual StdString AsDebugString() const;

    static VMEvaluationPrimitive* GetEvaluationPrimitive(int);
private:
    GCMethod* blockMethod;
    GCFrame* context;

    static const int VMBlockNumberOfFields;
};

#include "VMFrame.h"

void VMBlock::SetContext(VMFrame* contxt) {
    store_ptr(context, contxt);
}

VMFrame* VMBlock::GetContext() const {
    return load_ptr(context);
}
