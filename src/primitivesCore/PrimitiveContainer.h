#pragma once

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

#include <misc/defs.h>
#include <vmobjects/PrimitiveRoutine.h>

#include <map>

///Base class for all container objects holding SOM++ primitives.
//Primitive container classes need to initialize a std::map<StdString,
//PrimitiveRoutine*> in order to map smalltalk message names to the method
//to call.
class PrimitiveContainer {

public:
    PrimitiveContainer();
    virtual ~PrimitiveContainer();

    ///Every derived Class must use this method to initialize the methods
    //map with the mapping of a StdString with the smalltalk message
    //name and the corresponding functor object. The abstract functor object
    //class is defined in vmobjects/PrimitiveRoutine. Basically, the only
    //requirement for those objects is to implement:
    //  virtual void operator()(VMObject*, VMFrame*)
    virtual void SetPrimitive(const char* name, PrimitiveRoutine* routine);

    virtual PrimitiveRoutine* GetPrimitive(const std::string& routineName);

private:
    std::map<std::string, PrimitiveRoutine*>* methods;
};
