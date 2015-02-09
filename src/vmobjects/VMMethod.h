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

#include <iostream>

#include "VMInvokable.h"
#include "VMInteger.h"

class Interpreter;

class VMMethod: public VMInvokable {
    friend class Interpreter;

public:
    typedef GCMethod Stored;
    
    VMMethod(long bcCount, long numberOfConstants, long nof, Page*);

    inline  long      GetNumberOfLocals();
            void      SetNumberOfLocals(long nol, Page*);
            long      GetMaximumNumberOfStackElements();
            void      SetMaximumNumberOfStackElements(long stel, Page*);
    inline  long      GetNumberOfArguments();
#if GC_TYPE==PAUSELESS
    inline  long      GetNumberOfArgumentsGC();
#endif
            void      SetNumberOfArguments(long, Page*);
            long      GetNumberOfBytecodes();
    virtual void      SetHolder(VMClass* hld);
            void      SetHolderAll(VMClass* hld);
            vm_oop_t  GetConstant(long indx);
    inline  uint8_t   GetBytecode(long indx) const;
    inline  void      SetBytecode(long indx, uint8_t);

#ifdef UNSAFE_FRAME_OPTIMIZATION
    void SetCachedFrame(VMFrame* frame);
    VMFrame* GetCachedFrame() const;
#endif

#if GC_TYPE==PAUSELESS
    virtual void MarkReferences();
    virtual void CheckMarking(void (vm_oop_t));
#endif
    virtual void WalkObjects(walk_heap_fn, Page*);
    inline  long      GetNumberOfIndexableFields();
    virtual VMMethod* Clone(Page*);

    inline  void      SetIndexableField(long idx, vm_oop_t item);

    virtual void Invoke(Interpreter* interp, VMFrame* frame);

    void SetSignature(VMSymbol* sig, Page*);
    
    virtual void MarkObjectAsInvalid();

    virtual StdString AsDebugString();

private:
    inline uint8_t* GetBytecodes() const;
    inline vm_oop_t GetIndexableField(long idx);

    gc_oop_t numberOfLocals;
    gc_oop_t maximumNumberOfStackElements;
    gc_oop_t bcLength;
    gc_oop_t numberOfArguments;
    gc_oop_t numberOfConstants;

#ifdef UNSAFE_FRAME_OPTIMIZATION
    GCFrame* cachedFrame;
#endif

    gc_oop_t* indexableFields;
    uint8_t* bytecodes;
    static const long VMMethodNumberOfFields;
};
