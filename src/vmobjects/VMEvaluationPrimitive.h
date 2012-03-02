#pragma once
#ifndef VMEVALUATIONPRIMITIVE_H_
#define VMEVALUATIONPRIMITIVE_H_

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

#include "VMPrimitive.h"
#ifdef USE_TAGGING
class VMIntPointer;
#else
class VMInteger;
#endif
class VMObject;
class VMFrame;

class VMEvaluationPrimitive: public VMPrimitive {
public:
	VMEvaluationPrimitive(long argc);
#ifdef USE_TAGGING
	virtual void WalkObjects(AbstractVMObject* (AbstractVMObject*));
	virtual VMEvaluationPrimitive* Clone() const;
#else
	virtual void WalkObjects(pVMObject (pVMObject));
	virtual pVMEvaluationPrimitive Clone() const;
#endif
private:
	static pVMSymbol computeSignatureString(long argc);
	void evaluationRoutine(pVMObject object, pVMFrame frame);
	pVMInteger numberOfArguments;

};

#endif
