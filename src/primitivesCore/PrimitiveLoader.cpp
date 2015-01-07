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

#include <primitives/Array.h>
#include <primitives/Block.h>
#include <primitives/Class.h>
#include <primitives/Double.h>
#include <primitives/Integer.h>
#include <primitives/Method.h>
#include <primitives/Object.h>
#include <primitives/Primitive.h>
#include <primitives/String.h>
#include <primitives/Symbol.h>
#include <primitives/System.h>

#include "PrimitiveLoader.h"
#include "PrimitiveContainer.h"

#include <vmobjects/PrimitiveRoutine.h>

PrimitiveLoader PrimitiveLoader::loader;

PrimitiveLoader::PrimitiveLoader() {
    primitiveObjects = map<StdString, PrimitiveContainer*>();
    
    AddPrimitiveObject("Array",     new _Array());
    AddPrimitiveObject("Block",     new _Block());
    AddPrimitiveObject("Class",     new _Class());
    AddPrimitiveObject("Double",    new _Double());
    AddPrimitiveObject("Integer",   new _Integer());
    AddPrimitiveObject("Method",    new _Method());
    AddPrimitiveObject("Object",    new _Object());
    AddPrimitiveObject("Primitive", new _Primitive());
    AddPrimitiveObject("String",    new _String());
    AddPrimitiveObject("Symbol",    new _Symbol());
    AddPrimitiveObject("System",    new _System());
}

PrimitiveLoader::~PrimitiveLoader() {
    map<StdString, PrimitiveContainer*>::iterator it = primitiveObjects.begin();
    for (; it != primitiveObjects.end(); ++it) {
        delete it->second;
    }
}

void PrimitiveLoader::AddPrimitiveObject(const std::string& name,
        PrimitiveContainer* prim) {
    primitiveObjects[name] = prim;
}

bool PrimitiveLoader::supportsClass(const std::string& name) {
    return primitiveObjects[name] != nullptr;
}

bool PrimitiveLoader::SupportsClass(const std::string& name) {
    return loader.supportsClass(name);
}

PrimitiveRoutine* PrimitiveLoader::GetPrimitiveRoutine(const std::string& cname,
                                                       const std::string& mname, bool isPrimitive) {
    return loader.getPrimitiveRoutine(cname, mname, isPrimitive);
}

PrimitiveRoutine* PrimitiveLoader::getPrimitiveRoutine(const std::string& cname,
        const std::string& mname, bool isPrimitive) {
    PrimitiveRoutine* result;
    PrimitiveContainer* primitive = primitiveObjects[cname];
    if (!primitive) {
        Universe::ErrorPrint("Primitive object not found for name: " + cname + "\n");
        return nullptr;
    }
    result = primitive->GetPrimitive(mname);
    if (!result) {
        if (isPrimitive) {
            Universe::ErrorPrint("method " + mname + " not found in class " + cname + "\n");
        }
        return nullptr;
    }
    return result;
}

