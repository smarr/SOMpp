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


#include "ClassGenerationContext.h"

#include "../vmobjects/VMSymbol.h"
#include "../vmobjects/VMObject.h"
#include "../vmobjects/VMClass.h"

ClassGenerationContext::ClassGenerationContext() :
    instanceFields(), instanceMethods(), classFields(), classMethods(){
	name = NULL;
	superName = NULL;
    classSide = false;
}


ClassGenerationContext::~ClassGenerationContext() {
}


void ClassGenerationContext::AddClassField(pVMObject field) {
	this->classFields.Add(field);
}


void ClassGenerationContext::AddInstanceField(pVMObject field) {
	this->instanceFields.Add(field);
}


bool ClassGenerationContext::FindField(const StdString& field) {

	ExtendedList<pVMObject> fields = IsClassSide() ?
        classFields :
        instanceFields;
    return fields.IndexOf( (pVMObject)_UNIVERSE->SymbolFor(field)) != -1;

}



void ClassGenerationContext::AddInstanceMethod(pVMObject method) {
	this->instanceMethods.Add(method);
}



void ClassGenerationContext::AddClassMethod(pVMObject method) {
	this->classMethods.Add(method);
}


pVMClass ClassGenerationContext::Assemble() {
    // build class class name
    StdString ccname = string(name->GetStdString()) + " class";
    
    // Load the super class
    pVMClass superClass = _UNIVERSE->LoadClass(superName);
    
    // Allocate the class of the resulting class
    pVMClass resultClass = _UNIVERSE->NewClass(metaClassClass);

    // Initialize the class of the resulting class
    resultClass->SetInstanceFields(_UNIVERSE->NewArrayList(classFields));
    resultClass->SetInstanceInvokables(_UNIVERSE->NewArrayList(classMethods));
    resultClass->SetName(_UNIVERSE->SymbolFor(ccname));

    pVMClass superMClass = superClass->GetClass();
    resultClass->SetSuperClass(superMClass);
    
    // Allocate the resulting class
    pVMClass result = _UNIVERSE->NewClass(resultClass);
    
    // Initialize the resulting class
    result->SetInstanceFields(_UNIVERSE->NewArrayList(instanceFields));
    result->SetInstanceInvokables(_UNIVERSE->NewArrayList(instanceMethods));
    result->SetName(this->name);
    result->SetSuperClass(superClass);
    
    return result;
}



void ClassGenerationContext::AssembleSystemClass( pVMClass systemClass ) {
    systemClass->SetInstanceInvokables(_UNIVERSE->NewArrayList
                                                        (instanceMethods));
    systemClass->SetInstanceFields(_UNIVERSE->NewArrayList(instanceFields));
    // class-bound == class-instance-bound 
    pVMClass superMClass = systemClass->GetClass();
    superMClass->SetInstanceInvokables(_UNIVERSE->NewArrayList(classMethods));
    superMClass->SetInstanceFields(_UNIVERSE->NewArrayList(classFields));
}


