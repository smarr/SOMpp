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
#include <vmobjects/VMString.h>
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

void  _System::Global_(pVMObject /*object*/, pVMFrame frame) {
    pVMSymbol arg = (pVMSymbol)frame->Pop();
    /*pVMObject self = */
    frame->Pop();
    pVMObject result = _UNIVERSE->GetGlobal(arg);
    
    frame->Push( result ? result : nilObject);    
}


void  _System::Global_put_(pVMObject /*object*/, pVMFrame frame) {
    pVMObject value = frame->Pop();
    pVMSymbol arg = (pVMSymbol)frame->Pop();
    _UNIVERSE->SetGlobal(arg, value);    
}


void  _System::Load_(pVMObject /*object*/, pVMFrame frame) {
    pVMSymbol arg = (pVMSymbol)frame->Pop();
    /*pVMObject self = */
    frame->Pop();
    pVMClass result = _UNIVERSE->LoadClass(arg);

    frame->Push( result? (pVMObject)result : nilObject);
}


void  _System::Exit_(pVMObject /*object*/, pVMFrame frame) {
    pVMInteger err = (pVMInteger)frame->Pop();
    int32_t err_no = err->GetEmbeddedInteger();

    if(err_no != ERR_SUCCESS)
        frame->PrintStackTrace();
    _UNIVERSE->Quit(err_no);
}


void  _System::PrintString_(pVMObject /*object*/, pVMFrame frame) {
    pVMString arg = (pVMString)frame->Pop();
    std::string str = arg->GetStdString();
    cout << str;
}


void  _System::PrintNewline(pVMObject /*object*/, pVMFrame /*frame*/) {
    cout << endl;   
}


void  _System::Time(pVMObject /*object*/, pVMFrame frame) {
    /*pVMObject self = */
    frame->Pop();
    timeval* now = new timeval();

    gettimeofday(now, NULL);

    long long diff = 
        ((now->tv_sec - start_time->tv_sec) * 1000) + //seconds
        ((now->tv_usec - start_time->tv_usec) / 1000); // µseconds

    frame->Push((pVMObject)_UNIVERSE->NewInteger((int32_t)diff));

    delete(now);
}


void _System::FullGC(pVMObject /*object*/, pVMFrame frame) {
    // The stack will not be manipulated - leave it untouched.
    _UNIVERSE->FullGC();
}


_System::_System(void) : PrimitiveContainer() {
    start_time = new timeval();
    gettimeofday(start_time, NULL);

    this->SetPrimitive("global_", 
        static_cast<PrimitiveRoutine*>(new 
        Routine<_System>(this, &_System::Global_)));

    this->SetPrimitive("global_put_", 
        static_cast<PrimitiveRoutine*>(new 
        Routine<_System>(this, &_System::Global_put_)));

    this->SetPrimitive("load_", 
        static_cast<PrimitiveRoutine*>(new 
        Routine<_System>(this, &_System::Load_)));

    this->SetPrimitive("exit_", 
        static_cast<PrimitiveRoutine*>(new 
        Routine<_System>(this, &_System::Exit_)));

    this->SetPrimitive("printString_", 
        static_cast<PrimitiveRoutine*>(new 
        Routine<_System>(this, &_System::PrintString_)));

    this->SetPrimitive("printNewline", 
        static_cast<PrimitiveRoutine*>(new 
        Routine<_System>(this, &_System::PrintNewline)));

    this->SetPrimitive("time", 
        static_cast<PrimitiveRoutine*>(new 
        Routine<_System>(this, &_System::Time)));
    
    this->SetPrimitive("fullGC",
        static_cast<PrimitiveRoutine*>(new
        Routine<_System>(this, &_System::FullGC)));
}

_System::~_System()
{
    delete(start_time);
}


