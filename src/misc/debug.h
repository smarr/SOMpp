#pragma once

#ifndef DEBUG_H_
#define DEBUG_H_

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
#include <stdarg.h>


#define FprintfPass(f,x) \
    va_list ap; \
    va_start(ap, (x)); \
    (void)vfprintf((f), (x), ap); \
    va_end(ap)


static inline void DebugPrint(const char* fmt, ...) {
    FprintfPass(stderr, fmt);
}


static inline void DebugPrefix(const char* prefix) {
    DebugPrint("%-6s ", prefix);
}


#define DebugPass(x) \
    va_list ap; \
    va_start(ap, (x)); \
    (void)vfprintf(stderr, (x), ap); \
    va_end(ap)


static inline void DebugInfo(const char* fmt, ...) {
    #ifdef DEBUG
        DebugPrefix("INFO:"); 
        DebugPass(fmt);
    #else
        (void)fmt;
    #endif DEBUG
}


static inline void DebugLog(const char* fmt, ...) {
    #ifdef DEBUG
        DebugPrefix("LOG:"); 
        DebugPass(fmt);
    #else
    (void)fmt;
    #endif DEBUG
}


static inline void DebugWarn(const char* fmt, ...) {
    DebugPrefix("WARN:"); 
    DebugPass(fmt);
}


static inline void DebugError(const char* fmt, ...) {
    DebugPrefix("ERROR:"); 
    DebugPass(fmt);
}


static inline void DebugDump(const char* fmt, ...) {
    DebugPrefix("DUMP:"); 
    DebugPass(fmt);    
}


static inline void DebugTrace(const char* fmt, ...) {
    DebugPrefix("TRACE:"); 
    DebugPass(fmt);
}


#undef FprintfPass
#undef DebugPass 


#endif DEBUG_H_
