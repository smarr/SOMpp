#pragma once
#ifndef VMFREEOBJECT_H_
#define VMFREEOBJECT_H_

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

//This class represents unused memory in the heap.
//Its objectSize field states the size of the unused memory chunk.
//In order to build a double linked list of free memory chunks in the
//heap, some fields are used outside their normal purpose:
//The hash field is ("ab"-)used as the "next" pointer in the free-list.
//The numberOfFields field is ("ab"-)used as the "previous" pointer in
//the double-linked free-list.
//Unused objects are marked as -1 in the gc_field;
class VMFreeObject : public VMObject {
public:
    VMFreeObject();

    void SetNext(VMFreeObject* next);
    VMFreeObject* GetNext();
    void SetPrevious(VMFreeObject* prev);
    VMFreeObject* GetPrevious();
};


#endif


