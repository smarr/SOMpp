#include "../vmobjects/VMVector.h"

#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>

#include "../misc/defs.h"
#include "../vm/Print.h"
#include "../vmobjects/ObjectFormats.h"
#include "VMVector.h"

const size_t VMVector::VMVectorNumberOfFields = 3;

VMVector::VMVector(vm_oop_t first, vm_oop_t last, VMArray* storage)
    : VMObject(VMVectorNumberOfFields, sizeof(VMVector)),
      first(store_with_separate_barrier(first)),
      last(store_with_separate_barrier(last)),
      storage(store_with_separate_barrier(storage)) {
    static_assert(VMVectorNumberOfFields == 3);
    write_barrier(this, storage);
}

/* Rename as a more specifc error function */
void VMVector::IndexOutOfBounds(size_t idx, size_t size, string errorMessage) {
    // Construct the error message string
    std::string msg = errorMessage + " Index: " + std::to_string(idx) +
                      ", but vector size is only: " + std::to_string(size);

    // Create a SOM symbol for the error message
    vm_oop_t errorMsg = Universe::NewString(msg);

    // Arguments array for 'error:'
    vm_oop_t args[1] = { errorMsg };

    // Send 'error:' message to self (this VMVector)
    this->Send("error:", args, 1);

    // Execution shouldn't reach here if 'error:' raises an error in SOM
    ErrorExit("VMVector::IndexOutOfBounds: Execution should not reach after sending 'error'");

}

/* Send but then return the result of the send */
vm_oop_t VMVector::SendWithResult(const std::string& selectorString,
                                  vm_oop_t* arguments, size_t argc) {
  VMFrame* frame = Interpreter::GetFrame();
  VMSymbol* selector = SymbolFor(selectorString);

  frame->Push(this);
  for (size_t i = 0; i < argc; ++i) {
      frame->Push(arguments[i]);
  }

  VMClass* cl = GetClass();
  VMInvokable* invokable = cl->LookupInvokable(selector);

  invokable->Invoke(frame);

  return frame->Pop();
}
