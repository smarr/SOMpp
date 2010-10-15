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

#include "MethodGenerationContext.h"

#include "../interpreter/bytecodes.h"

#include "../vmobjects/VMSymbol.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/Signature.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMPrimitive.h"

MethodGenerationContext::MethodGenerationContext() {
	//signature = 0;
	holderGenc = 0;
	outerGenc = 0;
	this->arguments.Clear();
	this->literals.Clear();
	this->locals.Clear();
    this->bytecode.clear();
	primitive = false;
	blockMethod = false;
	finished = false;
}

pVMMethod MethodGenerationContext::Assemble() {
    // create a method instance with the given number of bytecodes and literals
    int numLiterals = this->literals.Size();
    
    pVMMethod meth = _UNIVERSE->NewMethod(this->signature, bytecode.size(),
                                                                numLiterals);
    
    // populate the fields that are immediately available
    int numLocals = this->locals.Size();
    meth->SetNumberOfLocals(numLocals);

    meth->SetMaximumNumberOfStackElements(this->ComputeStackDepth());

    // copy literals into the method
    for(int i = 0; i < numLiterals; i++) {
        pVMObject l = literals.Get(i);
        meth->SetIndexableField(i, l);
    }
    // copy bytecodes into method
    for(size_t i = 0; i < bytecode.size(); i++){
        meth->SetBytecode(i, bytecode[i]);
    }
    // return the method - the holder field is to be set later on!
    return meth;
}

pVMPrimitive MethodGenerationContext::AssemblePrimitive() {
    return VMPrimitive::GetEmptyPrimitive(this->signature);
}

MethodGenerationContext::~MethodGenerationContext() {
}

int8_t MethodGenerationContext::FindLiteralIndex(pVMObject lit) {
	return (int8_t)literals.IndexOf(lit);

}

bool MethodGenerationContext::FindVar(const StdString& var, int* index, 
                                        int* context, bool* isArgument) {
	if((*index = locals.IndexOf( var)) == -1) {
        if((*index = arguments.IndexOf( var)) == -1) {
            if(!outerGenc)
                return false;
            else {
                (*context)++;
				return outerGenc->FindVar(var, index,
                    context, isArgument);
            }
        } else
            *isArgument = true;
    }
    
    return true;
}

bool MethodGenerationContext::FindField(const StdString& field) {
	return holderGenc->FindField(field);
}

int MethodGenerationContext::GetNumberOfArguments() { 
    return arguments.Size(); 
}

uint8_t MethodGenerationContext::ComputeStackDepth() {
	uint8_t depth = 0;
    uint8_t maxDepth = 0;
    unsigned int i = 0;
    
    while(i < bytecode.size()) {
        switch(bytecode[i]) {
            case BC_HALT             :          i++;    break;
            case BC_DUP              : depth++; i++;    break;
            case BC_PUSH_LOCAL       :
            case BC_PUSH_ARGUMENT    : depth++; i += 3; break;
            case BC_PUSH_FIELD       :
            case BC_PUSH_BLOCK       :
            case BC_PUSH_CONSTANT    :
            case BC_PUSH_GLOBAL      : depth++; i += 2; break;
            case BC_POP              : depth--; i++;    break;
            case BC_POP_LOCAL        :
            case BC_POP_ARGUMENT     : depth--; i += 3; break;
            case BC_POP_FIELD        : depth--; i += 2; break;
            case BC_SEND             :
            case BC_SUPER_SEND       : {
                // these are special: they need to look at the number of
                // arguments (extractable from the signature)
                pVMSymbol sig = (pVMSymbol)literals.Get(bytecode[i + 1]);
                
                depth -= Signature::GetNumberOfArguments(sig);
                
				depth++; // return value
                i += 2;
                break;
            }
            case BC_RETURN_LOCAL     :
            case BC_RETURN_NON_LOCAL :          i++;    break;
            default                  :
                cout << "Illegal bytecode: " << bytecode[i];
                _UNIVERSE->Quit(1);
        }
        
        if(depth > maxDepth)
            maxDepth = depth;
    }
    
    return maxDepth;
}


void MethodGenerationContext::SetHolder(ClassGenerationContext* holder) {
	holderGenc = holder;
}

void MethodGenerationContext::SetOuter(MethodGenerationContext* outer) {
	outerGenc = outer;
}

void MethodGenerationContext::SetIsBlockMethod(bool isBlock) {
	blockMethod = isBlock;
}

void MethodGenerationContext::SetSignature(pVMSymbol sig) {
	signature = sig;
}

void MethodGenerationContext::SetPrimitive(bool prim) {
	primitive = prim;
}

void MethodGenerationContext::AddArgument(const StdString& arg) {
	arguments.PushBack(arg);
}

void MethodGenerationContext::AddLocal(const StdString& local) {
	locals.PushBack(local);
}

void MethodGenerationContext::AddLiteral(pVMObject lit) {
	literals.PushBack(lit);
}

bool MethodGenerationContext::AddArgumentIfAbsent(const StdString& arg) {
	if (locals.IndexOf( arg) != -1) return false;
	arguments.PushBack(arg);
	return true;
}

bool MethodGenerationContext::AddLocalIfAbsent(const StdString& local) {
	if (locals.IndexOf( local) != -1) return false;
	locals.PushBack(local);
	return true;
}

bool MethodGenerationContext::AddLiteralIfAbsent(pVMObject lit) {
	if (literals.IndexOf( lit) != -1) return false;
	literals.PushBack(lit);
	return true;
}
void MethodGenerationContext::SetFinished(bool finished) {
	this->finished = finished;
}

ClassGenerationContext* MethodGenerationContext::GetHolder() {
	return holderGenc;
}

MethodGenerationContext* MethodGenerationContext::GetOuter() {
	return outerGenc;
}

pVMSymbol MethodGenerationContext::GetSignature() {
	return signature;
}

bool MethodGenerationContext::IsPrimitive() {
	return primitive;
}

bool MethodGenerationContext::IsBlockMethod() {
	return blockMethod;
}

bool MethodGenerationContext::IsFinished() {
	return finished;
}

void MethodGenerationContext::AddBytecode(uint8_t bc) {
	bytecode.push_back(bc);
}
