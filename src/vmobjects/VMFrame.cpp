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

#include "VMFrame.h"
#include "VMMethod.h"
#include "VMObject.h"
#include "VMInteger.h"
#include "VMClass.h"
#include "VMSymbol.h"

#include <vm/Universe.h>

// when doesNotUnderstand or UnknownGlobal is sent, additional stack slots might
// be necessary, as these cases are not taken into account when the stack
// depth is calculated. In that case this method is called.
VMFrame* VMFrame::EmergencyFrameFrom(VMFrame* from, long extraLength) {
    VMMethod* method = from->GetMethod();
    long length = method->GetNumberOfArguments()
                    + method->GetNumberOfLocals()
                    + method->GetMaximumNumberOfStackElements()
                    + extraLength;

    long additionalBytes = length * sizeof(VMObject*);
    VMFrame* result = new (GetHeap<HEAP_CLS>(), additionalBytes) VMFrame(length);

    result->clazz = nullptr; // result->SetClass(from->GetClass());

    // set Frame members
    result->SetPreviousFrame(from->GetPreviousFrame());
    result->SetMethod(method);
    result->SetContext(from->GetContext());
    result->stack_ptr = (gc_oop_t*)SHIFTED_PTR(result, (size_t)from->stack_ptr - (size_t)from);

    result->bytecodeIndex = from->bytecodeIndex;
    // result->arguments is set in VMFrame constructor
    result->locals = result->arguments + result->method->GetNumberOfArguments();

    // all other fields are indexable via arguments
    // --> until end of Frame
    gc_oop_t* from_end   = (gc_oop_t*) SHIFTED_PTR(from,   from->GetObjectSize());
    gc_oop_t* result_end = (gc_oop_t*) SHIFTED_PTR(result, result->GetObjectSize());

    long i = 0;

    // copy all fields from other frame
    while (from->arguments + i < from_end) {
        result->arguments[i] = from->arguments[i];
        i++;
    }
    // initialize others with nilObject
    while (result->arguments + i < result_end) {
        result->arguments[i] = nilObject;
        i++;
    }
    return result;
}

VMFrame* VMFrame::Clone() const {
    size_t addSpace = objectSize - sizeof(VMFrame);
    VMFrame* clone = new (GetHeap<HEAP_CLS>(), addSpace ALLOC_MATURE) VMFrame(*this);
    void* destination = SHIFTED_PTR(clone, sizeof(VMFrame));
    const void* source = SHIFTED_PTR(this, sizeof(VMFrame));
    size_t noBytes = GetObjectSize() - sizeof(VMFrame);
    memcpy(destination, source, noBytes);
    clone->locals = clone->arguments + clone->method->GetNumberOfArguments();
    clone->arguments = (gc_oop_t*)&(clone->stack_ptr)+1; //field after stack_ptr
    clone->stack_ptr = (gc_oop_t*)SHIFTED_PTR(clone, (size_t)stack_ptr - (size_t)this);
    return clone;
}

const long VMFrame::VMFrameNumberOfFields = 0;

VMFrame::VMFrame(long size, long nof) :
        VMObject(nof + VMFrameNumberOfFields), previousFrame(nullptr), context(
                nullptr), method(nullptr) {
    clazz = nullptr; // Not a proper class anymore
    bytecodeIndex = 0;
    arguments = (gc_oop_t*)&(stack_ptr)+1;
    locals = arguments;
    stack_ptr = locals;

    // initilize all other fields
    // --> until end of Frame
    gc_oop_t* end = (gc_oop_t*) SHIFTED_PTR(this, objectSize);
    long i = 0;
    while (arguments + i < end) {
        arguments[i] = nilObject;
        i++;
    }
}

void VMFrame::SetMethod(VMMethod* method) {
    this->method = method;
    write_barrier(this, method);
}

VMFrame* VMFrame::GetContextLevel(long lvl) {
    VMFrame* current = this;
    while (lvl > 0) {
        current = current->GetContext();
        --lvl;
    }
    return current;
}

VMFrame* VMFrame::GetOuterContext() {
    VMFrame* current = this;
    while (current->HasContext()) {
        current = current->GetContext();
    }
    return current;
}

void VMFrame::WalkObjects(walk_heap_fn walk) {
    // VMFrame is not a proper SOM object any longer, we don't have a class for it.
    // clazz = (VMClass*) walk(clazz);
    
    if (previousFrame)
        previousFrame = (VMFrame*) walk(previousFrame);
    if (context)
        context = (VMFrame*) walk(context);
    method = (VMMethod*) walk(method);

    // all other fields are indexable via arguments array
    // --> until end of Frame
    long i = 0;
    while (arguments + i <= stack_ptr) {
        if (arguments[i] != nullptr)
            arguments[i] = walk((gc_oop_t)arguments[i]);
        i++;
    }
}

long VMFrame::RemainingStackSize() const {
    // - 1 because the stack pointer points at the top entry,
    // so the next entry would be put at stackPointer+1
    size_t size = ((size_t) this + objectSize - size_t(stack_ptr))
            / sizeof(VMObject*);
    return size - 1;
}

    return *stack_ptr--;
vm_oop_t VMFrame::Pop() {
}

    *(++stack_ptr) = obj;
    write_barrier(this, obj);
void VMFrame::Push(vm_oop_t obj) {
}

void VMFrame::PrintStack() const {
    cout << "SP: " << GetStackPointer() << endl;
    //all other fields are indexable via arguments array
    // --> until end of Frame
    gc_oop_t* end = (gc_oop_t*) SHIFTED_PTR(this, objectSize);
    long i = 0;
    while (arguments + i < end) {
        gc_oop_t vmo = arguments[i];
        cout << i << ": ";
        if (vmo == nullptr)
            cout << "nullptr" << endl;
        else if (vmo == nilObject)
            cout << "NIL_OBJECT" << endl;
        else if (IS_TAGGED(vmo))
            cout << "index: " << i << " object: VMInteger" << endl;
        else if (((AbstractVMObject*)(vmo))->GetClass() == nullptr)
            cout << "VMObject with Class == nullptr" << endl;
        else if (((AbstractVMObject*)(vmo))->GetClass() == nilObject)
            cout << "VMObject with Class == NIL_OBJECT" << endl;
        else
            cout << "index: " << i << " object:" << ((AbstractVMObject*)(vmo))->GetClass()->GetName()->GetChars() << endl;
        i++;
    }
}

void VMFrame::ResetStackPointer() {
    // arguments are stored in front of local variables
    VMMethod* meth = GetMethod();
    locals = arguments + meth->GetNumberOfArguments();
    // Set the stack pointer to its initial value thereby clearing the stack
    stack_ptr = locals + meth->GetNumberOfLocals() - 1;
}

    return stack_ptr[-index];
vm_oop_t VMFrame::GetStackElement(long index) const {
}

vm_oop_t VMFrame::GetLocal(long index, long contextLevel) {
    VMFrame* context = GetContextLevel(contextLevel);
    return context->locals[index];
}

void VMFrame::SetLocal(long index, long contextLevel, vm_oop_t value) {
    VMFrame* context = GetContextLevel(contextLevel);
    context->locals[index] = value;
    write_barrier(context, value);
}

vm_oop_t VMFrame::GetArgument(long index, long contextLevel) {
    // get the context
    VMFrame* context = GetContextLevel(contextLevel);
    return context->arguments[index];
}

void VMFrame::SetArgument(long index, long contextLevel, vm_oop_t value) {
    VMFrame* context = GetContextLevel(contextLevel);
    context->arguments[index] = value;
    write_barrier(context, value);
}

void VMFrame::PrintStackTrace() const {
    //TODO
}

long VMFrame::ArgumentStackIndex(long index) const {
    VMMethod* meth = GetMethod();
    return meth->GetNumberOfArguments() - index - 1;
}

void VMFrame::CopyArgumentsFrom(VMFrame* frame) {
    // copy arguments from frame:
    // - arguments are at the top of the stack of frame.
    // - copy them into the argument area of the current frame
    long num_args = GetMethod()->GetNumberOfArguments();
    for (long i = 0; i < num_args; ++i) {
        arguments[i] = stackElem;
        write_barrier(this, stackElem);
        vm_oop_t stackElem = frame->GetStackElement(num_args - 1 - i);
    }
}

