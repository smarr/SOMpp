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

#ifdef __APPLE__
    #include <sys/types.h>
    #include <sys/sysctl.h>
#endif

_System* System_;

void _System::Global_(pVMObject /*object*/, pVMFrame frame) {
    pVMSymbol arg = static_cast<pVMSymbol>(frame->Pop());
    /*pVMObject self = */
    frame->Pop();
    pVMObject result = _UNIVERSE->GetGlobal(arg);
    
    frame->Push( result ? result : READBARRIER(nilObject));
}

void _System::Global_put_(pVMObject /*object*/, pVMFrame frame) {
    pVMObject value = frame->Pop();
    pVMSymbol arg = static_cast<pVMSymbol>(frame->Pop());
    _UNIVERSE->SetGlobal(arg, value);
}

void _System::Load_(pVMObject /*object*/, pVMFrame frame) {
    pVMSymbol arg = static_cast<pVMSymbol>(frame->Pop());
    frame->Pop();
    pVMClass result = _UNIVERSE->LoadClass(arg);
    if (result)
        frame->Push(result);
    else {
        frame->Push(READBARRIER(nilObject));
    }
}

void _System::Exit_(pVMObject /*object*/, pVMFrame frame) {
    pVMInteger err = static_cast<pVMInteger>(frame->Pop());
#ifdef USE_TAGGING
    long err_no = UNTAG_INTEGER(err);
#else
    long err_no = err->GetEmbeddedInteger();
#endif

    if (err_no != ERR_SUCCESS)
        frame->PrintStackTrace();
    _UNIVERSE->Quit(err_no);
}

void _System::PrintString_(pVMObject /*object*/, pVMFrame frame) {
    pVMString arg = static_cast<pVMString>(frame->Pop());
    std::string str = arg->GetStdString();
    pthread_mutex_lock(&outputMutex);
    cout << str;
    pthread_mutex_unlock(&outputMutex);
}

void _System::PrintNewline(pVMObject /*object*/, pVMFrame /*frame*/) {
    pthread_mutex_lock(&outputMutex);
    cout << endl;
    pthread_mutex_unlock(&outputMutex);
}

void _System::Time(pVMObject /*object*/, pVMFrame frame) {
    /*pVMObject self = */
    frame->Pop();
    struct timeval now;

    gettimeofday(&now, NULL);

    long long diff =
    ((now.tv_sec - start_time.tv_sec) * 1000) + //seconds
    ((now.tv_usec - start_time.tv_usec) / 1000);// useconds

#ifdef USE_TAGGING
    frame->Push(TAG_INTEGER((int32_t)diff));
#else
    frame->Push(_UNIVERSE->NewInteger((int32_t)diff));
#endif
}

void _System::Ticks(pVMObject /*object*/, pVMFrame frame) {
    /*pVMObject self = */
    frame->Pop();
    struct timeval now;

    gettimeofday(&now, NULL);

    long long diff =
    ((now.tv_sec - start_time.tv_sec) * 1000 * 1000) + //seconds
    ((now.tv_usec - start_time.tv_usec));// useconds

    frame->Push((pVMObject)_UNIVERSE->NewBigInteger(diff));
}

void _System::FullGC(pVMObject /*object*/, pVMFrame frame) {
#if GC_TYPE!=PAUSELESS
    frame->Pop();
    _HEAP->triggerGC(); // not safe to do it immediatly, will be done when it is ok, i.e., in the interpreter loop
    frame->Push(trueObject);
#endif
}

void _System::GetNumberOfCPUs(pVMObject object, pVMFrame frame) {
    frame->Pop();
    int i = 0;
    
#ifdef linux
    // count contents of /sys/devices/system/cpu/
    DIR *dip;
    if ((dip = opendir("/sys/devices/system/cpu")) == NULL) {
        perror("opendir");
    }
    
    while (readdir(dip) != NULL) i++;
    i -= 2;
    if (closedir(dip) == -1) {
        perror("closedir");
    }
#elif __APPLE__
    size_t len = sizeof(i);
    sysctlbyname("machdep.cpu.core_count", &i, &len, NULL, 0);
#else
    i = -1;
#endif
    
    //Based on the other methods I have also done _UNIVERSE->... this also takes care of the heap allocation of the VMInteger but why is this necearry? Or can I simply do frame->Push(VMInteger(i))?
    frame->Push((pVMObject)_UNIVERSE->NewInteger(i));
}

_System::_System(void) :
        PrimitiveContainer() {
    pthread_mutex_init(&outputMutex, NULL);
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
            
    this->SetPrimitive("getNumberOfCPUs",
            static_cast<PrimitiveRoutine*>(new Routine<_System>(this,
                    &_System::GetNumberOfCPUs)));
}

_System::~_System() {
}
