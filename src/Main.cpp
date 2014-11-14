/*
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

#include <iostream>
#include <fstream>
#include <unistd.h>

#include "compiler/ClassGenerationContext.h"

#include "memory/PagedHeap.h"

#include "misc/ExtendedList.h"
#include "misc/defs.h"

#include "vm/Universe.h"

#include "vmobjects/VMObject.h"
#include "vmobjects/VMMethod.h"
#include "vmobjects/VMString.h"
#include "vmobjects/VMArray.h"
#include "vmobjects/ObjectFormats.h"

#include <misc/debug.h>
#include <sstream>

int main(int argc, char** argv) {
    sync_out(ostringstream() << "CWD: " << getwd(nullptr));


    sync_out(ostringstream() << "This is SOM++");
#if GC_TYPE==GENERATIONAL
    sync_out(ostringstream() << "\tgarbage collector: generational");
#elif GC_TYPE==COPYING
    sync_out(ostringstream() << "\tgarbage collector: copying");
#elif GC_TYPE==MARK_SWEEP
    sync_out(ostringstream() << "\tgarbage collector: mark-sweep");
#elif GC_TYPE==PAUSELESS
    sync_out(ostringstream() << "\tgarbage collector: pauseless");
#endif

#ifdef USE_TAGGING
    sync_out(ostringstream() << "\twith tagged integers");
#else
    sync_out(ostringstream() << "\tnot tagging integers");
#endif

#ifdef CACHE_INTEGER
    sync_out(ostringstream() << "\tcaching integers from " << INT_CACHE_MIN_VALUE
        << " to " << INT_CACHE_MAX_VALUE);
#else
    sync_out(ostringstream() << "\tnot caching integers");
#endif

    sync_out(ostringstream() << "--------------------------------------");


    Universe::Start(argc, argv);

    Universe::Quit(ERR_SUCCESS);
}

