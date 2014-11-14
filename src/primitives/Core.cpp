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

#include <string.h>

#include <vmobjects/PrimitiveRoutine.h>

#include "Array.h"
#include "BigInteger.h"
#include "Block.h"
#include "Class.h"
#include "Double.h"
#include "Integer.h"
#include "Method.h"
#include "Object.h"
#include "Primitive.h"
#include "String.h"
#include "Symbol.h"
#include "System.h"
#include "Thread.h"
#include "Mutex.h"
#include "Delay.h"
#include "Signal.h"

#include "../primitivesCore/PrimitiveContainer.h"
#include "../primitivesCore/PrimitiveLoader.h"

#if defined (_MSC_VER)
#include "Core.h"
#endif

static PrimitiveLoader* loader = nullptr;

void setup_primitives() {
    assert(loader == nullptr);
#ifdef __DEBUG
    cout << "Setting up the Core library" << endl;
#endif
    //Initialize loader
    loader = new PrimitiveLoader();
    loader->AddPrimitiveObject("Array",
            static_cast<PrimitiveContainer*>(new _Array()));

    loader->AddPrimitiveObject("BigInteger",
            static_cast<PrimitiveContainer*>(new _BigInteger()));

    loader->AddPrimitiveObject("Block",
            static_cast<PrimitiveContainer*>(new _Block()));

    loader->AddPrimitiveObject("Class",
            static_cast<PrimitiveContainer*>(new _Class()));

    loader->AddPrimitiveObject("Double",
            static_cast<PrimitiveContainer*>(new _Double()));

    loader->AddPrimitiveObject("Integer",
            static_cast<PrimitiveContainer*>(new _Integer()));
    
    loader->AddPrimitiveObject("Method",
            static_cast<PrimitiveContainer*>(new _Method()));

    loader->AddPrimitiveObject("Object",
            static_cast<PrimitiveContainer*>(new _Object()));
    
    loader->AddPrimitiveObject("Primitive",
            static_cast<PrimitiveContainer*>(new _Primitive()));

    loader->AddPrimitiveObject("String",
            static_cast<PrimitiveContainer*>(new _String()));

    loader->AddPrimitiveObject("Symbol",
            static_cast<PrimitiveContainer*>(new _Symbol()));

    loader->AddPrimitiveObject("System",
            static_cast<PrimitiveContainer*>(new _System()));
    
    loader->AddPrimitiveObject("Thread",
            static_cast<PrimitiveContainer*>(new _Thread()));
    
    loader->AddPrimitiveObject("Mutex",
            static_cast<PrimitiveContainer*>(new _Mutex()));
    
    loader->AddPrimitiveObject("Signal",
            static_cast<PrimitiveContainer*>(new _Signal()));
    
    loader->AddPrimitiveObject("Delay",
            static_cast<PrimitiveContainer*>(new _Delay()));
}


PrimitiveRoutine* get_primitive(const StdString& cname, const StdString& fname) {
    return loader->GetPrimitiveRoutine(cname, fname);
}
