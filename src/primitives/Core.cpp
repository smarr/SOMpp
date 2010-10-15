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
#include "Object.h"
#include "String.h"
#include "Symbol.h"
#include "System.h"


#include "../primitivesCore/PrimitiveContainer.h"
#include "../primitivesCore/PrimitiveLoader.h"

#if defined (_MSC_VER)
#include "Core.h"
#endif


static PrimitiveLoader* loader = NULL;

//"Constructor"
//#define __DEBUG
extern "C" void setup() {
    if (!loader) {
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

        loader->AddPrimitiveObject("Object", 
            static_cast<PrimitiveContainer*>(new _Object()));

        loader->AddPrimitiveObject("String", 
            static_cast<PrimitiveContainer*>(new _String()));

        loader->AddPrimitiveObject("Symbol", 
            static_cast<PrimitiveContainer*>(new _Symbol()));

        loader->AddPrimitiveObject("System", 
            static_cast<PrimitiveContainer*>(new _System()));
    }
}

extern "C" bool supportsClass(const char* name) {
    if (!loader) setup();
    return loader->SupportsClass(name);
}



extern "C" void tearDown() {
    
    if (loader) delete loader;
    
}

//"Factory method"
extern "C" PrimitiveRoutine* create(const StdString& cname, const StdString& fname) {

#ifdef __DEBUG
    cout << "Loading PrimitiveContainer: " << cname << "::" << fname << endl;
#endif
    if (!loader) setup();
    return loader->GetPrimitiveRoutine(cname, fname);
}

/* Lib initialization */
#ifdef __GNUC__
void init(void) __attribute__((constructor));
void fini(void) __attribute__((destructor));
#else
void _init(void);
void _fini(void);
#pragma init _init
#pragma fini _fini
#endif __GNUC__


#ifdef __GNUC__
void init(void)
#else
void _init(void)
#endif __GNUC__
{
    // Call init funcions.
    setup();
    
}


#ifdef __GNUC__
void fini(void)
#else
void _fini(void)
#endif __GNUC__
{
    tearDown();
}
