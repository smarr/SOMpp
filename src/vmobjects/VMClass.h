#pragma once
#ifndef VMCLASS_H_
#define VMCLASS_H_

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


#include <vector>

#include "VMObject.h"

#include "../misc/defs.h"


#if defined(_MSC_VER)   //Visual Studio
    #include <windows.h> 
    #include "../primitives/Core.h"
#endif

class VMSymbol;
class VMArray;
class VMPrimitive;
class ClassGenerationContext;

class VMClass : public VMObject {
public:
	VMClass();
    VMClass(int numberOfFields);

	virtual inline pVMClass  GetSuperClass() const; 
    virtual inline void      SetSuperClass(pVMClass); 
    virtual bool             HasSuperClass() const;  
    virtual inline pVMSymbol GetName() const; 
    virtual inline void      SetName(pVMSymbol);  
    virtual inline pVMArray  GetInstanceFields() const; 
    virtual inline void      SetInstanceFields(pVMArray); 
    virtual inline pVMArray  GetInstanceInvokables() const; 
    virtual void      SetInstanceInvokables(pVMArray); 
    virtual int       GetNumberOfInstanceInvokables() const; 
    virtual pVMObject GetInstanceInvokable(int) const; 
    virtual void      SetInstanceInvokable(int, pVMObject); 
    virtual pVMObject LookupInvokable(pVMSymbol) const; 
    virtual int       LookupFieldIndex(pVMSymbol) const; 
    virtual bool      AddInstanceInvokable(pVMObject); 
    virtual void      AddInstancePrimitive(pVMPrimitive); 
    virtual pVMSymbol GetInstanceFieldName(int)const; 
    virtual int       GetNumberOfInstanceFields() const; 
    virtual bool      HasPrimitives() const; 
    virtual void      LoadPrimitives(const vector<StdString>&);
	

private:
	
    
    StdString genLoadstring(const StdString& cp, 
                       const StdString& cname
                       ) const;

    StdString genCoreLoadstring(const StdString& cp) const;

    void* loadLib(const StdString& path) const;
    bool isResponsible(void* handle, const StdString& cl) const;
    void setPrimitives(void* handle, const StdString& cname);
   
    
    int numberOfSuperInstanceFields() const;

	pVMClass  superClass; 
    pVMSymbol name; 
    pVMArray  instanceFields; 
    pVMArray  instanceInvokables;

    static const int VMClassNumberOfFields;
};


pVMClass VMClass::GetSuperClass() const {
	return superClass;
}


void VMClass::SetSuperClass(pVMClass sup) {
	superClass = sup;
}


pVMSymbol VMClass::GetName()  const {
	return name;
}


void VMClass::SetName(pVMSymbol nam) {
	name = nam;
}


pVMArray VMClass::GetInstanceFields() const {
	return instanceFields;
}


void VMClass::SetInstanceFields(pVMArray instFields) {
	instanceFields = instFields;
}


pVMArray VMClass::GetInstanceInvokables() const {
	return instanceInvokables;
}

#endif
