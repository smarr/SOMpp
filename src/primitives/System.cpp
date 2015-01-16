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

void _System::Global_(VMObject* /*object*/, VMFrame* frame) {
    VMSymbol* arg = static_cast<VMSymbol*>(frame->Pop());
    /*VMObject* self = */
    frame->Pop();
    VMObject* result = GetUniverse()->GetGlobal(arg);
    
    frame->Push( result ? result : READBARRIER(nilObject));
}

void _System::Global_put_(VMObject* /*object*/, VMFrame* frame) {
    VMObject* value = frame->Pop();
    VMSymbol* arg = static_cast<VMSymbol*>(frame->Pop());
    GetUniverse()->SetGlobal(arg, value);
}

void _System::Load_(VMObject* /*object*/, VMFrame* frame) {
    VMSymbol* arg = static_cast<VMSymbol*>(frame->Pop());
    frame->Pop();
    VMClass* result = GetUniverse()->LoadClass(arg);
    if (result)
        frame->Push(result);
    else {
        frame->Push(READBARRIER(nilObject));
    }
}

void _System::Exit_(VMObject* /*object*/, VMFrame* frame) {
    VMInteger* err = static_cast<VMInteger*>(frame->Pop());
#ifdef USE_TAGGING
    long err_no = UNTAG_INTEGER(err);
#else
    long err_no = err->GetEmbeddedInteger();
#endif

    if (err_no != ERR_SUCCESS)
        frame->PrintStackTrace();
    GetUniverse()->Quit(err_no);
}

void _System::PrintString_(VMObject* /*object*/, VMFrame* frame) {
    VMString* arg = static_cast<VMString*>(frame->Pop());
    std::string str = arg->GetStdString();
    pthread_mutex_lock(&outputMutex);
    cout << str;
    pthread_mutex_unlock(&outputMutex);
}

void _System::PrintNewline(VMObject* /*object*/, VMFrame* /*frame*/) {
    pthread_mutex_lock(&outputMutex);
    cout << endl;
    pthread_mutex_unlock(&outputMutex);
}

void _System::Time(VMObject* /*object*/, VMFrame* frame) {
    /*VMObject* self = */
    frame->Pop();
    struct timeval now;

    gettimeofday(&now, NULL);

    long long diff =
    ((now.tv_sec - start_time.tv_sec) * 1000) + //seconds
    ((now.tv_usec - start_time.tv_usec) / 1000);// useconds

#ifdef USE_TAGGING
    frame->Push(TAG_INTEGER((int32_t)diff));
#else
    frame->Push(GetUniverse()->NewInteger((int32_t)diff));
#endif
}

void _System::Ticks(VMObject* /*object*/, VMFrame* frame) {
    /*VMObject* self = */
    frame->Pop();
    struct timeval now;

    gettimeofday(&now, NULL);

    long long diff =
    ((now.tv_sec - start_time.tv_sec) * 1000 * 1000) + //seconds
    ((now.tv_usec - start_time.tv_usec));// useconds

    frame->Push((VMObject*)GetUniverse()->NewBigInteger(diff));
}

void _System::FullGC(VMObject* /*object*/, VMFrame* frame) {
#if GC_TYPE!=PAUSELESS
    frame->Pop();
    _HEAP->triggerGC(); // not safe to do it immediatly, will be done when it is ok, i.e., in the interpreter loop
    frame->Push(READBARRIER(trueObject));
#endif
}

void _System::GetNumberOfCPUs(VMObject* object, VMFrame* frame) {
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
    
    //Based on the other methods I have also done GetUniverse()->... this also takes care of the heap allocation of the VMInteger but why is this necearry? Or can I simply do frame->Push(VMInteger(i))?
    frame->Push((VMObject*)GetUniverse()->NewInteger(i));
}

_System::_System(void) : PrimitiveContainer() {
    pthread_mutex_init(&outputMutex, NULL);
    gettimeofday(&start_time, NULL);
    SetPrimitive("global_",         new Routine<_System>(this, &_System::Global_));
    SetPrimitive("global_put_",     new Routine<_System>(this, &_System::Global_put_));
    SetPrimitive("load_",           new Routine<_System>(this, &_System::Load_));
    SetPrimitive("exit_",           new Routine<_System>(this, &_System::Exit_));
    SetPrimitive("printString_",    new Routine<_System>(this, &_System::PrintString_));
    SetPrimitive("printNewline",    new Routine<_System>(this, &_System::PrintNewline));
    SetPrimitive("time",            new Routine<_System>(this, &_System::Time));
    SetPrimitive("ticks",           new Routine<_System>(this, &_System::Ticks));
    SetPrimitive("fullGC",          new Routine<_System>(this, &_System::FullGC));
    SetPrimitive("getNumberOfCPUs", new Routine<_System>(this, &_System::GetNumberOfCPUs));
}

_System::~_System() {}
