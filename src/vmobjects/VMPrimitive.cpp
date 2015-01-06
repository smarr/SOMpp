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

#include "VMPrimitive.h"
#include "VMSymbol.h"
#include "VMClass.h"

#include "../vm/Universe.h"

//needed to instanciate the Routine object for the  empty routine
#include "../primitivesCore/Routine.h"

VMPrimitive* VMPrimitive::GetEmptyPrimitive(VMSymbol* sig, bool classSide) {
    VMPrimitive* prim = new (GetHeap<HEAP_CLS>()) VMPrimitive(sig);
    prim->empty = true;
    prim->SetRoutine(new Routine<VMPrimitive>(prim, &VMPrimitive::EmptyRoutine, classSide));
    return prim;
}

const int VMPrimitive::VMPrimitiveNumberOfFields = 2;

VMPrimitive::VMPrimitive(VMSymbol* signature) : VMInvokable(VMPrimitiveNumberOfFields) {
    //the only class that explicitly does this.
    SetClass(load_ptr(primitiveClass));
    SetSignature(signature);
    routine = nullptr;
    empty = false;
}

VMPrimitive* VMPrimitive::Clone() const {
    VMPrimitive* prim = new (GetHeap<HEAP_CLS>(), 0 ALLOC_MATURE) VMPrimitive(*this);
    return prim;
}

void VMPrimitive::WalkObjects(walk_heap_fn walk) {
    clazz     = static_cast<GCClass*>(walk(clazz));
    signature = static_cast<GCSymbol*>(walk(signature));
    holder    = static_cast<GCClass*>(walk(holder));
}

void VMPrimitive::EmptyRoutine(Interpreter*, VMFrame*) {
    VMSymbol* sig = GetSignature();
    Universe::ErrorPrint("undefined primitive called: " + sig->GetStdString() + "\n");
}

StdString VMPrimitive::AsDebugString() const {
    return "Primitive(" + GetClass()->GetName()->GetStdString() + ">>#"
                        + GetSignature()->GetStdString() + ")";
}

