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

void _System::Global_(pVMObject /*object*/, pVMFrame frame) {
    pVMSymbol arg = static_cast<pVMSymbol>(frame->Pop());
    /*pVMObject self = */
    frame->Pop();
    pVMObject result = GetUniverse()->GetGlobal(arg);

    frame->Push( result ? result : nilObject);
}

void _System::Global_put_(pVMObject /*object*/, pVMFrame frame) {
    pVMObject value = frame->Pop();
    pVMSymbol arg = static_cast<pVMSymbol>(frame->Pop());
    GetUniverse()->SetGlobal(arg, value);
}

void _System::Load_(pVMObject /*object*/, pVMFrame frame) {
    pVMSymbol arg = static_cast<pVMSymbol>(frame->Pop());
    frame->Pop();
    pVMClass result = GetUniverse()->LoadClass(arg);
    if (result)
        frame->Push(result);
    else
        frame->Push(nilObject);
}

void _System::Exit_(pVMObject /*object*/, pVMFrame frame) {
    pVMInteger err = static_cast<pVMInteger>(frame->Pop());

    long err_no = INT_VAL(err);
    if (err_no != ERR_SUCCESS)
        frame->PrintStackTrace();
    GetUniverse()->Quit(err_no);
}

void _System::PrintString_(pVMObject /*object*/, pVMFrame frame) {
    pVMString arg = static_cast<pVMString>(frame->Pop());
    std::string str = arg->GetStdString();
    cout << str;
}

void _System::PrintNewline(pVMObject /*object*/, pVMFrame /*frame*/) {
    cout << endl;
}

void _System::Time(pVMObject /*object*/, pVMFrame frame) {
    /*pVMObject self = */
    frame->Pop();
    struct timeval now;

    gettimeofday(&now, NULL);

    long long diff =
    ((now.tv_sec - start_time.tv_sec) * 1000) + //seconds
    ((now.tv_usec - start_time.tv_usec) / 1000);// useconds

    frame->Push(NEW_INT((int32_t)diff));
}

void _System::Ticks(pVMObject /*object*/, pVMFrame frame) {
    /*pVMObject self = */
    frame->Pop();
    struct timeval now;

    gettimeofday(&now, NULL);

    long long diff =
    ((now.tv_sec - start_time.tv_sec) * 1000 * 1000) + //seconds
    ((now.tv_usec - start_time.tv_usec));// useconds

    frame->Push((pVMObject)GetUniverse()->NewBigInteger(diff));
}

void _System::FullGC(pVMObject /*object*/, pVMFrame frame) {
    frame->Pop();
    GetHeap<HEAP_CLS>()->triggerGC(); // not safe to do it immediatly, will be done when it is ok, i.e., in the interpreter loop
    frame->Push(trueObject);
}

_System::_System(void) :
        PrimitiveContainer() {
    gettimeofday(&start_time, NULL);

    this->SetPrimitive("global_",
            static_cast<PrimitiveRoutine*>(new Routine<_System>(this,
                    &_System::Global_)));

    this->SetPrimitive("global_put_",
            static_cast<PrimitiveRoutine*>(new Routine<_System>(this,
                    &_System::Global_put_)));

    this->SetPrimitive("load_",
            static_cast<PrimitiveRoutine*>(new Routine<_System>(this,
                    &_System::Load_)));

    this->SetPrimitive("exit_",
            static_cast<PrimitiveRoutine*>(new Routine<_System>(this,
                    &_System::Exit_)));

    this->SetPrimitive("printString_",
            static_cast<PrimitiveRoutine*>(new Routine<_System>(this,
                    &_System::PrintString_)));

    this->SetPrimitive("printNewline",
            static_cast<PrimitiveRoutine*>(new Routine<_System>(this,
                    &_System::PrintNewline)));

    this->SetPrimitive("time",
            static_cast<PrimitiveRoutine*>(new Routine<_System>(this,
                    &_System::Time)));

    this->SetPrimitive("ticks",
            static_cast<PrimitiveRoutine*>(new Routine<_System>(this,
                    &_System::Ticks)));

    this->SetPrimitive("fullGC",
            static_cast<PrimitiveRoutine*>(new Routine<_System>(this,
                    &_System::FullGC)));
}

_System::~_System() {
}
