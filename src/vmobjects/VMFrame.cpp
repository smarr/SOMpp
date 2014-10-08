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
#include "../interpreter/Interpreter.h"

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
#if GC_TYPE==GENERATIONAL
    pVMFrame result = new (_HEAP, _PAGE, additionalBytes) VMFrame(length);
#elif GC_TYPE==PAUSELESS
    pVMFrame result = new (_PAGE, additionalBytes) VMFrame(length);
#else
    pVMFrame result = new (_HEAP, additionalBytes) VMFrame(length);
#endif

    result->clazz = nullptr; // result->SetClass(from->GetClass());

    // set Frame members
    result->SetPreviousFrame(from->GetPreviousFrame());
    result->SetMethod(method);
    result->SetContext(from->GetContext());
    result->stack_ptr = (pVMObject*)SHIFTED_PTR(result, (size_t)from->stack_ptr - (size_t)from);

    result->bytecodeIndex = from->bytecodeIndex;
    // result->arguments is set in VMFrame constructor
    result->locals = result->arguments + result->GetMethod()->GetNumberOfArguments();

    // all other fields are indexable via arguments
    // --> until end of Frame
    pVMObject* from_end   = (pVMObject*) SHIFTED_PTR(from,   from->GetObjectSize());
    pVMObject* result_end = (pVMObject*) SHIFTED_PTR(result, result->GetObjectSize());

    long i = 0;

    // copy all fields from other frame
    while (from->arguments + i < from_end) {
        PG_HEAP(ReadBarrier((void**)(&from->arguments[i])));
        result->arguments[i] = from->arguments[i];
        i++;
    }
    // initialize others with nilObject
    while (result->arguments + i < result_end) {
        PG_HEAP(ReadBarrier((void**)(&nilObject)));
        result->arguments[i] = nilObject;
        i++;
    }
    return result;
}

#if GC_TYPE==GENERATIONAL
pVMFrame VMFrame::Clone() {
    size_t addSpace = objectSize - sizeof(VMFrame);
    pVMFrame clone = new (_HEAP, _PAGE, addSpace, true) VMFrame(*this);
    void* destination = SHIFTED_PTR(clone, sizeof(VMFrame));
    const void* source = SHIFTED_PTR(this, sizeof(VMFrame));
    size_t noBytes = GetObjectSize() - sizeof(VMFrame);
    memcpy(destination, source, noBytes);
    clone->arguments = (pVMObject*)&(clone->stack_ptr)+1; //field after stack_ptr
    clone->locals = clone->arguments + clone->method->GetNumberOfArguments();
    clone->stack_ptr = (pVMObject*)SHIFTED_PTR(clone, (size_t)stack_ptr - (size_t)this);
    return clone;
}
#elif GC_TYPE==PAUSELESS
pVMFrame VMFrame::Clone(Page* page) {
    size_t addSpace = objectSize - sizeof(VMFrame);
    pVMFrame clone = new (page, addSpace) VMFrame(*this);
    void* destination = SHIFTED_PTR(clone, sizeof(VMFrame));
    const void* source = SHIFTED_PTR(this, sizeof(VMFrame));
    size_t noBytes = GetObjectSize() - sizeof(VMFrame);
    memcpy(destination, source, noBytes);
    clone->arguments = (pVMObject*)&(clone->stack_ptr)+1; //field after stack_ptr
    clone->locals = clone->arguments + clone->method->GetNumberOfArguments();
    clone->stack_ptr = (pVMObject*)SHIFTED_PTR(clone, (size_t)stack_ptr - (size_t)this);
    return clone;
}
#else
pVMFrame VMFrame::Clone() {
    size_t addSpace = objectSize - sizeof(VMFrame);
    pVMFrame clone = new (_HEAP, addSpace) VMFrame(*this);
    void* destination = SHIFTED_PTR(clone, sizeof(VMFrame));
    const void* source = SHIFTED_PTR(this, sizeof(VMFrame));
    size_t noBytes = GetObjectSize() - sizeof(VMFrame);
    memcpy(destination, source, noBytes);
    clone->arguments = (pVMObject*)&(clone->stack_ptr)+1; //field after stack_ptr
    clone->locals = clone->arguments + clone->method->GetNumberOfArguments();
    clone->stack_ptr = (pVMObject*)SHIFTED_PTR(clone, (size_t)stack_ptr - (size_t)this);
    return clone;
}
#endif


const long VMFrame::VMFrameNumberOfFields = 0;

VMFrame::VMFrame(long size, long nof) :
        VMObject(nof + VMFrameNumberOfFields), previousFrame(NULL), context(
                NULL), method(NULL) {
    clazz = nullptr; // Not a proper class anymore
    bytecodeIndex = 0;
    arguments = (pVMObject*)&(stack_ptr)+1;
    locals = arguments;
    stack_ptr = locals;

    // initilize all other fields
    // --> until end of Frame
    pVMObject* end = (pVMObject*) SHIFTED_PTR(this, objectSize);
    long i = 0;
    while (arguments + i < end) {
        PG_HEAP(ReadBarrier((void**)(&nilObject)));
        arguments[i] = nilObject;
        i++;
    }
}

void VMFrame::SetMethod(pVMMethod method) {
    this->method = method;
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, method);
#endif
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

long VMFrame::RemainingStackSize() const {
    // - 1 because the stack pointer points at the top entry,
    // so the next entry would be put at stackPointer+1
    size_t size = ((size_t) this + objectSize - size_t(stack_ptr))
            / sizeof(pVMObject);
    return size - 1;
}

pVMObject VMFrame::Pop() {
    PG_HEAP(ReadBarrier((void**)stack_ptr));
    return *stack_ptr--;
}

void VMFrame::Push(pVMObject obj) {
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, (VMOBJECT_PTR)obj);
#endif
    *(++stack_ptr) = obj;
}

void VMFrame::PrintStack() const {
    cout << "SP: " << this->GetStackPointer() << endl;
    //all other fields are indexable via arguments array
    // --> until end of Frame
    pVMObject* end = (pVMObject*) SHIFTED_PTR(this, objectSize);
    long i = 0;
    while (arguments + i < end) {
        PG_HEAP(ReadBarrier((void**)(&arguments[i])));
        pVMObject vmo = arguments[i];
        cout << i << ": ";
        if (UNTAG_REFERENCE(vmo) == NULL)
        cout << "NULL" << endl;
        PG_HEAP(ReadBarrier((void**)(&nilObject)));
        if (UNTAG_REFERENCE(vmo) == UNTAG_REFERENCE(nilObject))
        cout << "NIL_OBJECT" << endl;
#ifdef USE_TAGGING
        if (IS_TAGGED(vmo)) {
            cout << "index: " << i << " object: VMInteger" << endl;
        }
        else {
            if (UNTAG_REFERENCE(((VMOBJECT_PTR)vmo)->GetClass()) == NULL)
            cout << "VMObject with Class == NULL" << endl;
            PG_HEAP(Readbarrier((void**)&nilObject));
            if (UNTAG_REFERENCE(((VMOBJECT_PTR)vmo)->GetClass()) == nilObject)
            cout << "VMObject with Class == NIL_OBJECT" << endl;
            else
            cout << "index: " << i << " object:"
            << ((VMOBJECT_PTR)vmo)->GetClass()->GetName()->GetChars() << endl;
        }
#else
        if (UNTAG_REFERENCE(vmo->GetClass()) == NULL)
        cout << "VMObject with Class == NULL" << endl;
        PG_HEAP(ReadBarrier((void**)(&nilObject)));
        if (UNTAG_REFERENCE(vmo->GetClass()) == UNTAG_REFERENCE(nilObject))
        cout << "VMObject with Class == NIL_OBJECT" << endl;
        else
        cout << "index: " << i << " object:"
        << vmo->GetClass()->GetName()->GetChars() << endl;
#endif
        i++;
    }
}

void VMFrame::ResetStackPointer() {
    // arguments are stored in front of local variables
    pVMMethod meth = this->GetMethod();
    locals = arguments + meth->GetNumberOfArguments();
    // Set the stack pointer to its initial value thereby clearing the stack
    stack_ptr = locals + meth->GetNumberOfLocals() - 1;
}

pVMObject VMFrame::GetStackElement(long index) const {
    PG_HEAP(ReadBarrier((void**)(&stack_ptr[-index])));
    return stack_ptr[-index];
}

void VMFrame::SetStackElement(long index, pVMObject obj) {
    stack_ptr[-index] = obj;
}

pVMObject VMFrame::GetLocal(long index, long contextLevel) {
    pVMFrame context = this->GetContextLevel(contextLevel);
    PG_HEAP(ReadBarrier((void**)(&context->locals[index])));
    return context->locals[index];
}

void VMFrame::SetLocal(long index, long contextLevel, pVMObject value) {
    pVMFrame context = this->GetContextLevel(contextLevel);
    context->locals[index] = value;
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(context, (VMOBJECT_PTR)value);
#endif
}

pVMObject VMFrame::GetArgument(long index, long contextLevel) {
    // get the context
    pVMFrame context = this->GetContextLevel(contextLevel);
    PG_HEAP(ReadBarrier((void**)(&context->arguments[index])));
    return context->arguments[index];
}

void VMFrame::SetArgument(long index, long contextLevel, pVMObject value) {
    pVMFrame context = this->GetContextLevel(contextLevel);
    context->arguments[index] = value;
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(context, (VMOBJECT_PTR)value);
#endif
}

void VMFrame::PrintStackTrace() const {
    //TODO
}

long VMFrame::ArgumentStackIndex(long index) /*const*/ {
    pVMMethod meth = this->GetMethod();
    return meth->GetNumberOfArguments() - index - 1;
}

void VMFrame::CopyArgumentsFrom(pVMFrame frame) {
    // copy arguments from frame:
    // - arguments are at the top of the stack of frame.
    // - copy them into the argument area of the current frame
    long num_args = GetMethod()->GetNumberOfArguments();
    for (long i = 0; i < num_args; ++i) {
        pVMObject stackElem = frame->GetStackElement(num_args - 1 - i);
        arguments[i] = stackElem;
#if GC_TYPE==GENERATIONAL
        _HEAP->WriteBarrier(this, (VMOBJECT_PTR)stackElem);
#endif
    }
}

#if GC_TYPE==PAUSELESS
void VMFrame::MarkReferences() {
    if (previousFrame)
        ReadBarrierForGCThread((void**)&previousFrame);
    if (context)
        ReadBarrierForGCThread((void**)&context);
    ReadBarrierForGCThread((void**)&method);
    
    long i = 0;
    while (arguments + i <= stack_ptr) {
        if (arguments[i] != NULL)
            ReadBarrierForGCThread((void**)&arguments[i]);
        i++;
    }
}
#else
void VMFrame::WalkObjects(VMOBJECT_PTR (*walk)(VMOBJECT_PTR)) {
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
        if (arguments[i] != NULL)
            arguments[i] = walk((VMOBJECT_PTR)arguments[i]);
        i++;
    }
}
#endif
