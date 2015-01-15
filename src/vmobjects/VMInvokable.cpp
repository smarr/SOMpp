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

#include "VMInvokable.h"
#include "VMSymbol.h"
#include "VMClass.h"

bool VMInvokable::IsPrimitive() const {
    return false;
}

VMSymbol* VMInvokable::GetSignature() {
    return READBARRIER(signature);
}

void VMInvokable::SetSignature(VMSymbol* sig) {
    signature = WRITEBARRIER(sig);
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, sig);
#endif
}

VMClass* VMInvokable::GetHolder() {
    return READBARRIER(holder);
}

void VMInvokable::SetHolder(VMClass* hld) {
    holder = WRITEBARRIER(hld);
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, hld);
#endif
}

void VMInvokable::MarkObjectAsInvalid() {
    VMObject::MarkObjectAsInvalid();
    signature = (GCSymbol*) INVALID_GC_POINTER;
    holder = (GCClass*) INVALID_GC_POINTER;
}

#if GC_TYPE==PAUSELESS
void VMInvokable::MarkReferences() {
    ReadBarrierForGCThread(&clazz);
    ReadBarrierForGCThread(&signature);
    ReadBarrierForGCThread(&holder);
}
void VMInvokable::CheckMarking(void (*walk)(AbstractVMObject*)) {
    assert(GetNMTValue(clazz) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(clazz));
    walk(Untag(clazz));
    assert(GetNMTValue(signature) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(signature));
    walk(Untag(signature));
    if (holder) {
        assert(GetNMTValue(holder) == _HEAP->GetGCThread()->GetExpectedNMT());
        CheckBlocked(Untag(holder));
        walk(Untag(holder));
    }
}
#else
void VMInvokable::WalkObjects(VMOBJECT_PTR (*walk)(VMOBJECT_PTR)) {
    clazz = (GCClass*) (walk(READBARRIER(clazz)));
    signature = (GCSymbol*) (walk(READBARRIER(signature)));
    if (holder)
        holder = (GCClass*) (walk(READBARRIER(holder)));
}
#endif