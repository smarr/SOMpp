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

#include "PrimitiveContainer.h"

#include <cassert>
#include <iostream>
#include <map>
#include <ostream>
#include <string>

#include "../vm/Symbols.h"
#include "../vmobjects/PrimitiveRoutine.h"
#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMPrimitive.h"
#include "../vmobjects/VMSafePrimitive.h"
#include "../vmobjects/VMSymbol.h"
#include "Primitives.h"

void PrimitiveContainer::SetPrimitive(const char* name,
                                      PrimitiveRoutine* routine) {
    methods[std::string(name)] = routine;
}

void PrimitiveContainer::Add(const char* name,
                             BinaryPrimitiveRoutine routine,
                             bool classSide) {
    binaryPrims[std::string(name)] = {routine, classSide};
}

void PrimitiveContainer::Add(const char* name,
                             UnaryPrimitiveRoutine routine,
                             bool classSide) {
    unaryPrims[std::string(name)] = {routine, classSide};
}

PrimitiveRoutine* PrimitiveContainer::GetPrimitive(
    const std::string& routineName) {
    if (methods.find(routineName) != methods.end()) {
        return methods[routineName];
    }
    return nullptr;
}

void PrimitiveContainer::InstallPrimitives(VMClass* clazz, bool classSide) {
    for (auto const& p : unaryPrims) {
        assert(p.second.IsValid());
        if (classSide != p.second.isClassSide) {
            continue;
        }

        VMSymbol* sig = SymbolFor(p.first);
        if (clazz->AddInstanceInvokable(
                VMSafePrimitive::GetSafeUnary(sig, p.second))) {
            cout << "Warn: Primitive " << p.first
                 << " is not in class definition for class "
                 << clazz->GetName()->GetStdString() << endl;
        }
    }

    for (auto const& p : binaryPrims) {
        assert(p.second.IsValid());
        if (classSide != p.second.isClassSide) {
            continue;
        }

        VMSymbol* sig = SymbolFor(p.first);
        if (clazz->AddInstanceInvokable(
                VMSafePrimitive::GetSafeBinary(sig, p.second))) {
            cout << "Warn: Primitive " << p.first
                 << " is not in class definition for class "
                 << clazz->GetName()->GetStdString() << endl;
        }
    }

    for (auto const& p : methods) {
        if (classSide != p.second->isClassSide()) {
            continue;
        }

        VMSymbol* sig = SymbolFor(p.first);
        VMPrimitive* prim = VMPrimitive::GetEmptyPrimitive(sig, classSide);
        prim->SetRoutine(p.second, false);
        if (clazz->AddInstanceInvokable(prim)) {
            cout << "Warn: Primitive " << p.first
                 << " is not in class definition for class "
                 << clazz->GetName()->GetStdString() << endl;
        }
    }
}
