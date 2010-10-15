#pragma once
#ifndef ROUTINE_H_
#define ROUTINE_H_

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


#include <vmobjects/PrimitiveRoutine.h>

///Implementation for a functor class with PrimitiveRoutine as base class.
//It stores an object and a pointer to one of its methods. It is invoked
//by calling the Routine's poerator "()".
template <class TClass> class Routine : public PrimitiveRoutine
{
private:
  void (TClass::*fpt)(pVMObject, pVMFrame);   // pointer to member function
  TClass* pt2Object;                          // pointer to object

public:

  // constructor - takes pointer to an object and pointer to a member and stores
  // them in two private variables
   Routine(TClass* _pt2Object, void(TClass::*_fpt)(pVMObject, pVMFrame)) : 
       PrimitiveRoutine() { 
       pt2Object = _pt2Object;  
       fpt=_fpt; 
   };

  // override operator "()"
  virtual void operator()(pVMObject obj, pVMFrame frm) {
      (*pt2Object.*fpt)(obj, frm);// execute member function
  };              

};

#endif
