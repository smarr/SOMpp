#pragma once
#ifndef VMARRAY_H_
#define VMARRAY_H_

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


#include "VMObject.h"
#include "VMInteger.h"

class VMArray : public VMObject {
	public:
		VMArray(int size, int32_t nof=0);
#ifdef USE_TAGGING
		virtual void 		WalkObjects(AbstractVMObject* (AbstractVMObject*));
#else
		virtual void 		WalkObjects(pVMObject (pVMObject));
#endif
		virtual int         GetNumberOfIndexableFields() const;
		pVMArray    CopyAndExtendWith(pVMObject) const;
		pVMObject   GetIndexableField(int32_t idx) const;
		void        SetIndexableField(int32_t idx, pVMObject value);
		void        CopyIndexableFieldsTo(pVMArray) const;
#ifdef USE_TAGGING
		virtual VMArray* Clone() const;
#else
		virtual pVMArray Clone() const;
#endif

	private:
		static const int VMArrayNumberOfFields;
};
#endif
