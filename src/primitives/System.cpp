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

#include <stdio.h>

#include <time.h>

#include <vmobjects/VMObject.h>
#include <vmobjects/VMFrame.h>
#include <vmobjects/VMSymbol.h>
#include <vmobjects/VMClass.h>
#include <vmobjects/VMInteger.h>

#include <vm/Universe.h>

#include "System.h"
#include "../primitivesCore/Routine.h"

#if defined(__GNUC__)

#include <sys/time.h>

#else

#include <misc/gettimeofday.h>

#endif

_System* System_;

void _System::Global_(Interpreter*, VMFrame* frame) {
    VMSymbol* arg = static_cast<VMSymbol*>(frame->Pop());
    /*VMObject* self = */
    frame->Pop();
    vm_oop_t result = GetUniverse()->GetGlobal(arg);

    frame->Push(result ? result : load_ptr(nilObject));
}

void _System::Global_put_(Interpreter*, VMFrame* frame) {
    vm_oop_t value = frame->Pop();
    VMSymbol* arg = static_cast<VMSymbol*>(frame->Pop());
    GetUniverse()->SetGlobal(arg, value);
}

void _System::HasGlobal_(Interpreter*, VMFrame* frame) {
    VMSymbol* arg = static_cast<VMSymbol*>(frame->Pop());
    frame->Pop(); // pop self (system)

    if (GetUniverse()->HasGlobal(arg)) {
        frame->Push(load_ptr(trueObject));
    } else {
        frame->Push(load_ptr(falseObject));
    }
}

void _System::Load_(Interpreter*, VMFrame* frame) {
    VMSymbol* arg = static_cast<VMSymbol*>(frame->Pop());
    frame->Pop();
    VMClass* result = GetUniverse()->LoadClass(arg);
    if (result)
        frame->Push(result);
    else
        frame->Push(load_ptr(nilObject));
}

void _System::Exit_(Interpreter*, VMFrame* frame) {
    vm_oop_t err = frame->Pop();

    long err_no = INT_VAL(err);
    if (err_no != ERR_SUCCESS)
        frame->PrintStackTrace();
    GetUniverse()->Quit(err_no);
}

void _System::PrintString_(Interpreter*, VMFrame* frame) {
    VMString* arg = static_cast<VMString*>(frame->Pop());
    std::string str = arg->GetStdString();
    Universe::Print(str);
}

void _System::PrintNewline(Interpreter*, VMFrame*) {
    Universe::Print("\n");
}

void _System::PrintNewline_(Interpreter*, VMFrame* frame) {
    VMString* arg = static_cast<VMString*>(frame->Pop());
    std::string str = arg->GetStdString();
    Universe::Print(str + "\n");
}


void _System::Time(Interpreter*, VMFrame* frame) {
    /*VMObject* self = */
    frame->Pop();
    struct timeval now;

    gettimeofday(&now, nullptr);

    long long diff =
    ((now.tv_sec - start_time.tv_sec) * 1000) + //seconds
    ((now.tv_usec - start_time.tv_usec) / 1000);// useconds

    frame->Push(NEW_INT(diff));
}

void _System::Ticks(Interpreter*, VMFrame* frame) {
    /*VMObject* self = */
    frame->Pop();
    struct timeval now;

    gettimeofday(&now, nullptr);

    long long diff =
    ((now.tv_sec - start_time.tv_sec) * 1000 * 1000) + //seconds
    ((now.tv_usec - start_time.tv_usec));// useconds

    frame->Push(NEW_INT(diff));
}

void _System::FullGC(Interpreter*, VMFrame* frame) {
    frame->Pop();
    GetHeap<HEAP_CLS>()->triggerGC(); // not safe to do it immediatly, will be done when it is ok, i.e., in the interpreter loop
    frame->Push(load_ptr(trueObject));
}

_System::_System(void) : PrimitiveContainer() {
    gettimeofday(&start_time, nullptr);

    SetPrimitive("global_",      new Routine<_System>(this, &_System::Global_,     false));
    SetPrimitive("global_put_",  new Routine<_System>(this, &_System::Global_put_, false));
    SetPrimitive("hasGlobal_",   new Routine<_System>(this, &_System::HasGlobal_,  false));
    SetPrimitive("load_",        new Routine<_System>(this, &_System::Load_, false));
    SetPrimitive("exit_",        new Routine<_System>(this, &_System::Exit_, false));
    SetPrimitive("printString_", new Routine<_System>(this, &_System::PrintString_, false));
    SetPrimitive("printNewline", new Routine<_System>(this, &_System::PrintNewline, false));
    SetPrimitive("printNewline_",new Routine<_System>(this, &_System::PrintNewline_, false));
    SetPrimitive("time",         new Routine<_System>(this, &_System::Time,   false));
    SetPrimitive("ticks",        new Routine<_System>(this, &_System::Ticks,  false));
    SetPrimitive("fullGC",       new Routine<_System>(this, &_System::FullGC, false));
}

_System::~_System() {}
