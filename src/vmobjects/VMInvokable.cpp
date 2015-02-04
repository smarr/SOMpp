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

#warning VMInvokable needs a GetHash implementation

bool VMInvokable::IsPrimitive() const {
    return false;
}

VMSymbol* VMInvokable::GetSignature() {
    return load_ptr(signature);
}

void VMInvokable::SetSignature(VMSymbol* sig) {
    store_ptr(signature, sig);
}

VMClass* VMInvokable::GetHolder() {
    return load_ptr(holder);
}

void VMInvokable::SetHolder(VMClass* hld) {
    store_ptr(holder, hld);
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
void VMInvokable::CheckMarking(void (*walk)(vm_oop_t)) {
    assert(GetNMTValue(clazz) == GetHeap<HEAP_CLS>()->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(clazz));
    walk(Untag(clazz));
    assert(GetNMTValue(signature) == GetHeap<HEAP_CLS>()->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(signature));
    walk(Untag(signature));
    if (holder) {
        assert(GetNMTValue(holder) == GetHeap<HEAP_CLS>()->GetGCThread()->GetExpectedNMT());
        CheckBlocked(Untag(holder));
        walk(Untag(holder));
    }
}
#else
void VMInvokable::WalkObjects(walk_heap_fn walk, Page* page) {
    clazz     = static_cast<GCClass*>(walk(clazz, page));
    signature = static_cast<GCSymbol*>(walk(signature, page));
    if (holder) {
        holder = static_cast<GCClass*>(walk(holder, page));
    }
}
#endif