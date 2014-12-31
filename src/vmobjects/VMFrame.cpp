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
pVMFrame VMFrame::EmergencyFrameFrom(pVMFrame from, long extraLength) {
    pVMMethod method = from->GetMethod();
    long length = method->GetNumberOfArguments()
                    + method->GetNumberOfLocals()
                    + method->GetMaximumNumberOfStackElements()
                    + extraLength;

    long additionalBytes = length * sizeof(pVMObject);
    pVMFrame result = new (GetHeap<HEAP_CLS>(), additionalBytes) VMFrame(length);

    result->clazz = nullptr; // result->SetClass(from->GetClass());

    // set Frame members
    result->SetPreviousFrame(from->GetPreviousFrame());
    result->SetMethod(method);
    result->SetContext(from->GetContext());
    result->stack_ptr = (oop_t*)SHIFTED_PTR(result, (size_t)from->stack_ptr - (size_t)from);

    result->bytecodeIndex = from->bytecodeIndex;
    // result->arguments is set in VMFrame constructor
    result->locals = result->arguments + result->method->GetNumberOfArguments();

    // all other fields are indexable via arguments
    // --> until end of Frame
    oop_t* from_end   = (oop_t*) SHIFTED_PTR(from,   from->GetObjectSize());
    oop_t* result_end = (oop_t*) SHIFTED_PTR(result, result->GetObjectSize());

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

pVMFrame VMFrame::Clone() const {
    size_t addSpace = objectSize - sizeof(VMFrame);
    pVMFrame clone = new (GetHeap<HEAP_CLS>(), addSpace ALLOC_MATURE) VMFrame(*this);
    void* destination = SHIFTED_PTR(clone, sizeof(VMFrame));
    const void* source = SHIFTED_PTR(this, sizeof(VMFrame));
    size_t noBytes = GetObjectSize() - sizeof(VMFrame);
    memcpy(destination, source, noBytes);
    clone->arguments = (oop_t*)&(clone->stack_ptr)+1; //field after stack_ptr
    clone->locals = clone->arguments + clone->method->GetNumberOfArguments();
    clone->stack_ptr = (oop_t*)SHIFTED_PTR(clone, (size_t)stack_ptr - (size_t)this);
    return clone;
}

const long VMFrame::VMFrameNumberOfFields = 0;

VMFrame::VMFrame(long size, long nof) :
        VMObject(nof + VMFrameNumberOfFields), previousFrame(nullptr), context(
                nullptr), method(nullptr) {
    clazz = nullptr; // Not a proper class anymore
    bytecodeIndex = 0;
    arguments = (oop_t*)&(stack_ptr)+1;
    locals = arguments;
    stack_ptr = locals;

    // initilize all other fields
    // --> until end of Frame
    oop_t* end = (oop_t*) SHIFTED_PTR(this, objectSize);
    long i = 0;
    while (arguments + i < end) {
        arguments[i] = nilObject;
        i++;
    }
}

void VMFrame::SetMethod(pVMMethod method) {
    this->method = method;
    write_barrier(this, method);
}

pVMFrame VMFrame::GetContextLevel(long lvl) {
    pVMFrame current = this;
    while (lvl > 0) {
        current = current->GetContext();
        --lvl;
    }
    return current;
}

pVMFrame VMFrame::GetOuterContext() {
    pVMFrame current = this;
    while (current->HasContext()) {
        current = current->GetContext();
    }
    return current;
}

void VMFrame::WalkObjects(oop_t (*walk)(oop_t)) {
    // VMFrame is not a proper SOM object any longer, we don't have a class for it.
    // clazz = (pVMClass) walk(clazz);
    
    if (previousFrame)
        previousFrame = (pVMFrame) walk(previousFrame);
    if (context)
        context = (pVMFrame) walk(context);
    method = (pVMMethod) walk(method);

    // all other fields are indexable via arguments array
    // --> until end of Frame
    long i = 0;
    while (arguments + i <= stack_ptr) {
        if (arguments[i] != nullptr)
            arguments[i] = walk((oop_t)arguments[i]);
        i++;
    }
}

long VMFrame::RemainingStackSize() const {
    // - 1 because the stack pointer points at the top entry,
    // so the next entry would be put at stackPointer+1
    size_t size = ((size_t) this + objectSize - size_t(stack_ptr))
            / sizeof(pVMObject);
    return size - 1;
}

oop_t VMFrame::Pop() {
    return *stack_ptr--;
}

void VMFrame::Push(oop_t obj) {
    *(++stack_ptr) = obj;
    write_barrier(this, obj);
}

void VMFrame::PrintStack() const {
    cout << "SP: " << GetStackPointer() << endl;
    //all other fields are indexable via arguments array
    // --> until end of Frame
    oop_t* end = (oop_t*) SHIFTED_PTR(this, objectSize);
    long i = 0;
    while (arguments + i < end) {
        oop_t vmo = arguments[i];
        cout << i << ": ";
        if (vmo == nullptr)
            cout << "nullptr" << endl;
        else if (vmo == nilObject)
            cout << "NIL_OBJECT" << endl;
        else if (IS_TAGGED(vmo))
            cout << "index: " << i << " object: VMInteger" << endl;
        else if (pVMAbstract(vmo)->GetClass() == nullptr)
            cout << "VMObject with Class == nullptr" << endl;
        else if (pVMAbstract(vmo)->GetClass() == nilObject)
            cout << "VMObject with Class == NIL_OBJECT" << endl;
        else
            cout << "index: " << i << " object:" << pVMAbstract(vmo)->GetClass()->GetName()->GetChars() << endl;
        i++;
    }
}

void VMFrame::ResetStackPointer() {
    // arguments are stored in front of local variables
    pVMMethod meth = GetMethod();
    locals = arguments + meth->GetNumberOfArguments();
    // Set the stack pointer to its initial value thereby clearing the stack
    stack_ptr = locals + meth->GetNumberOfLocals() - 1;
}

oop_t VMFrame::GetStackElement(long index) const {
    return stack_ptr[-index];
}

oop_t VMFrame::GetLocal(long index, long contextLevel) {
    pVMFrame context = GetContextLevel(contextLevel);
    return context->locals[index];
}

void VMFrame::SetLocal(long index, long contextLevel, oop_t value) {
    pVMFrame context = GetContextLevel(contextLevel);
    context->locals[index] = value;
    write_barrier(context, value);
}

oop_t VMFrame::GetArgument(long index, long contextLevel) {
    // get the context
    pVMFrame context = GetContextLevel(contextLevel);
    return context->arguments[index];
}

void VMFrame::SetArgument(long index, long contextLevel, oop_t value) {
    pVMFrame context = GetContextLevel(contextLevel);
    context->arguments[index] = value;
    write_barrier(context, value);
}

void VMFrame::PrintStackTrace() const {
    //TODO
}

long VMFrame::ArgumentStackIndex(long index) const {
    pVMMethod meth = GetMethod();
    return meth->GetNumberOfArguments() - index - 1;
}

void VMFrame::CopyArgumentsFrom(pVMFrame frame) {
    // copy arguments from frame:
    // - arguments are at the top of the stack of frame.
    // - copy them into the argument area of the current frame
    long num_args = GetMethod()->GetNumberOfArguments();
    for (long i = 0; i < num_args; ++i) {
        oop_t stackElem = frame->GetStackElement(num_args - 1 - i);
        arguments[i] = stackElem;
        write_barrier(this, stackElem);
    }
}

