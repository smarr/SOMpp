/*
 * AbstractVMObject.cpp
 *
 *  Created on: 10.03.2011
 *      Author: christian
 */

#include "AbstractObject.h"

#include <vm/Universe.h>

#include "VMFrame.h"
#include "VMClass.h"
#include "VMInvokable.h"

#include <algorithm>
#include <numeric>

std::vector<uintptr_t>
ComputeSlotMaps(uintptr_t* const root, const vector<uintptr_t*>& fields)
{
  uintptr_t base = 0;
  uintptr_t slotMap = 0;
  
  std::vector<uintptr_t> slotMaps;  

  for(auto& field : fields) {
    // 1 byte or 8 bytes? assume 1 byte for now, change if incorrect.
    uintptr_t ptrdiff = field - root - base;
    
    if(ptrdiff >= sizeof(uintptr_t) * 8) {
      slotMaps.push_back(slotMap);
      slotMap = 0;
      
      while(ptrdiff >= sizeof(uintptr_t) * 8) {
	slotMaps.push_back(slotMap);
	
	base += sizeof(uintptr_t) * 8;
	ptrdiff -= sizeof(uintptr_t) * 8;
      }
    }

    slotMap |= (1 << ptrdiff);
  }

  slotMaps.push_back(slotMap);
  std::reverse(slotMaps.begin(), slotMaps.end());  
  return slotMaps;
}

size_t AbstractVMObject::GetHash() {
    return (size_t) this;
}

void AbstractVMObject::Send(Interpreter* interp, StdString selectorString, vm_oop_t* arguments, long argc) {
    VMFrame* frame = interp->GetFrame();
    VMSymbol* selector = GetUniverse()->SymbolFor(selectorString);
    frame->Push(this);

    for (long i = 0; i < argc; ++i) {
        frame->Push(arguments[i]);
    }

    VMClass* cl = GetClass();
    VMInvokable* invokable = cl->LookupInvokable(selector);
    invokable->Invoke(interp, frame);
}

long AbstractVMObject::GetFieldIndex(VMSymbol* fieldName) const {
    return GetClass()->LookupFieldIndex(fieldName);
}
