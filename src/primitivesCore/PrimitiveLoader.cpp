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


#include "PrimitiveLoader.h"
#include "PrimitiveContainer.h"

#include <vmobjects/PrimitiveRoutine.h>


PrimitiveLoader::PrimitiveLoader() {
    primitiveObjects =  map<StdString, PrimitiveContainer*>();
}

PrimitiveLoader::~PrimitiveLoader() {
    map<StdString, PrimitiveContainer*>::iterator it = primitiveObjects.begin();
    for (; it != primitiveObjects.end(); ++it) {
        delete it->second;
    }
   
}

void PrimitiveLoader::AddPrimitiveObject( const char* name, PrimitiveContainer* prim) {
    primitiveObjects[StdString(name)] = prim;
}

bool PrimitiveLoader::SupportsClass( const char* name ) {
    return primitiveObjects[StdString(name)] != NULL;
}

PrimitiveRoutine* PrimitiveLoader::GetPrimitiveRoutine( const std::string& cname, const std::string& mname ) {
    PrimitiveRoutine* result; 
    PrimitiveContainer* primitive = primitiveObjects[cname];
    if (!primitive) {
        cout << "Primitive object not found for name: " << cname << endl;
        return NULL;
    }
    result = primitive->GetPrimitive(mname);
    if (!result) {
        cout << "method " << mname << " not found in class" << cname << endl;
        return NULL;
    }
    return result;
}

