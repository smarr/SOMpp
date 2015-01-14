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

#include "../memory/pauseless/PauselessHeap.h"
#include "VMPrimitive.h"
#include "VMSymbol.h"
#include "VMClass.h"

#include "../interpreter/Interpreter.h"
#include "../vm/Universe.h"

//needed to instanciate the Routine object for the  empty routine
#include "../primitivesCore/Routine.h"

pVMPrimitive VMPrimitive::GetEmptyPrimitive( pVMSymbol sig ) {
#if GC_TYPE==GENERATIONAL
    pVMPrimitive prim = new (_HEAP, _PAGE) VMPrimitive(sig);
#elif GC_TYPE==PAUSELESS
    pVMPrimitive prim = new (_HEAP, _UNIVERSE->GetInterpreter(), 0, true) VMPrimitive(sig);
#else
    pVMPrimitive prim = new (_HEAP) VMPrimitive(sig);
#endif
    prim->empty = true;
    prim->SetRoutine(new Routine<VMPrimitive>(prim, &VMPrimitive::EmptyRoutine));
    return prim;
}

const int VMPrimitive::VMPrimitiveNumberOfFields = 2;

VMPrimitive::VMPrimitive(pVMSymbol signature) : VMInvokable(VMPrimitiveNumberOfFields) {
    //the only class that explicitly does this.
    this->SetClass(READBARRIER(primitiveClass));

    this->SetSignature(signature);
    this->routine = NULL;
    this->empty = false;
}

#if GC_TYPE==GENERATIONAL
pVMPrimitive VMPrimitive::Clone() {
    return new (_HEAP, _PAGE, 0, true) VMPrimitive(*this);
}
#elif GC_TYPE==PAUSELESS
pVMPrimitive VMPrimitive::Clone(Interpreter* thread) {
    pVMPrimitive clone = new (_HEAP, thread) VMPrimitive(*this);
    /* clone->IncreaseVersion();
    this->MarkObjectAsInvalid(); */
    return clone;
}
pVMPrimitive VMPrimitive::Clone(PauselessCollectorThread* thread) {
    pVMPrimitive clone = new (_HEAP, thread) VMPrimitive(*this);
    /* clone->IncreaseVersion();
    this->MarkObjectAsInvalid(); */
    return clone;
}
#else
pVMPrimitive VMPrimitive::Clone() {
    return new (_HEAP) VMPrimitive(*this);
}
#endif

void VMPrimitive::EmptyRoutine( pVMObject _self, pVMFrame /*frame*/) {
    pVMInvokable self = static_cast<pVMInvokable>(_self);
    pVMSymbol sig = self->GetSignature();
    cout << "undefined primitive called: " << sig->GetChars() << endl;
}

#if GC_TYPE==PAUSELESS
void VMPrimitive::MarkReferences() {
    ReadBarrierForGCThread(&clazz);
    ReadBarrierForGCThread(&signature);
    ReadBarrierForGCThread(&holder);
}
void VMPrimitive::CheckMarking(void (*walk)(AbstractVMObject*)) {
    assert(GetNMTValue(clazz) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(clazz));
    walk(Untag(clazz));
    assert(GetNMTValue(signature) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(signature));
    walk(Untag(signature));
    assert(GetNMTValue(holder) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(holder));
    walk(Untag(holder));
}
#else
void VMPrimitive::WalkObjects(VMOBJECT_PTR (*walk)(VMOBJECT_PTR)) {
    clazz     = (GCClass*)(walk(READBARRIER(clazz)));
    signature = (GCSymbol*)(walk(READBARRIER(signature)));
    holder    = (GCClass*)(walk(READBARRIER(holder)));
}
#endif

void VMPrimitive::MarkObjectAsInvalid() {
    VMInvokable::MarkObjectAsInvalid();
}
