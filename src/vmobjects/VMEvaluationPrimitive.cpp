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

#include "VMEvaluationPrimitive.h"
#include "VMSymbol.h"
#include "VMObject.h"
#include "VMFrame.h"
#include "VMBlock.h"
#include "VMInteger.h"

#include "../vm/Universe.h"

//needed to instanciate the Routine object for the evaluation routine
#include "../primitivesCore/Routine.h"


VMEvaluationPrimitive::VMEvaluationPrimitive(int argc) : 
                       VMPrimitive(computeSignatureString(argc)) {
    this->SetRoutine(new Routine<VMEvaluationPrimitive>(this, 
                               &VMEvaluationPrimitive::evaluationRoutine));
    this->SetEmpty(false);
#ifdef USE_TAGGING
    this->numberOfArguments = argc;
#else
    this->numberOfArguments = _UNIVERSE->NewInteger(argc);
#endif
#if GC_TYPE==GENERATIONAL
	_HEAP->writeBarrier(this, numberOfArguments);
#endif
}

#ifdef USE_TAGGING
VMEvaluationPrimitive* VMEvaluationPrimitive::Clone() const {
#else
pVMEvaluationPrimitive VMEvaluationPrimitive::Clone() const {
#endif
#if GC_TYPE==GENERATIONAL
	pVMEvaluationPrimitive evPrim = new (_HEAP, 0, true) VMEvaluationPrimitive(*this);
#else
	pVMEvaluationPrimitive evPrim = new (_HEAP) VMEvaluationPrimitive(*this);
#endif
	return evPrim;
}


#ifdef USE_TAGGING
void VMEvaluationPrimitive::WalkObjects(AbstractVMObject*
		(*walk)(AbstractVMObject*)) {
	VMPrimitive::WalkObjects(walk);
	numberOfArguments = (VMInteger*)walk(numberOfArguments);
#else
void VMEvaluationPrimitive::WalkObjects(pVMObject (*walk)(pVMObject)) {
	VMPrimitive::WalkObjects(walk);
	numberOfArguments = (pVMInteger)walk(numberOfArguments);
#endif
#if GC_TYPE==GENERATIONAL
	_HEAP->writeBarrier(this, numberOfArguments);
#endif
}


pVMSymbol VMEvaluationPrimitive::computeSignatureString(int argc){
#define VALUE_S "value"
#define VALUE_LEN 5
#define WITH_S    "with:"
#define WITH_LEN (4+1)
#define COLON_S ":"
    
    StdString signatureString;
    
    // Compute the signature string
    if(argc==1) {
        signatureString += VALUE_S;
    } else {
        signatureString += VALUE_S ;
        signatureString += COLON_S; 
        --argc;
        while(--argc) 
            // Add extra value: selector elements if necessary
            signatureString += WITH_S;
    }

    // Return the signature string
    return _UNIVERSE->SymbolFor(signatureString);
}

void VMEvaluationPrimitive::evaluationRoutine(pVMObject object, pVMFrame frame){
    pVMEvaluationPrimitive self = (pVMEvaluationPrimitive) object;

     // Get the block (the receiver) from the stack
#ifdef USE_TAGGING
    int numArgs = (int32_t)self->numberOfArguments;
#else
    int numArgs = self->numberOfArguments->GetEmbeddedInteger();
#endif
    pVMBlock block = (pVMBlock) frame->GetStackElement(numArgs - 1);
    
    // Get the context of the block...
    pVMFrame context = block->GetContext();
    
    // Push a new frame and set its context to be the one specified in the block
    pVMFrame NewFrame = _UNIVERSE->GetInterpreter()->PushNewFrame(
                                                        block->GetMethod());
    NewFrame->CopyArgumentsFrom(frame);
    NewFrame->SetContext(context);
}
