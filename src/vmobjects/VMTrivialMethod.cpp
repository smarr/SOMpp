#include "VMTrivialMethod.h"

#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

#include "../compiler/BytecodeGenerator.h"
#include "../compiler/MethodGenerationContext.h"
#include "../compiler/Variable.h"
#include "../memory/Heap.h"
#include "../misc/defs.h"
#include "../vm/LogAllocation.h"
#include "../vm/Universe.h"
#include "AbstractObject.h"
#include "ObjectFormats.h"
#include "VMFrame.h"

VMTrivialMethod* MakeLiteralReturn(VMSymbol* sig, vector<Variable>& arguments,
                                   vm_oop_t literal) {
    VMLiteralReturn* result =
        new (GetHeap<HEAP_CLS>(), 0) VMLiteralReturn(sig, arguments, literal);
    LOG_ALLOCATION("VMLiteralReturn", result->GetObjectSize());
    return result;
}

VMTrivialMethod* MakeGlobalReturn(VMSymbol* sig, vector<Variable>& arguments,
                                  VMSymbol* globalName) {
    VMGlobalReturn* result =
        new (GetHeap<HEAP_CLS>(), 0) VMGlobalReturn(sig, arguments, globalName);
    LOG_ALLOCATION("VMGlobalReturn", result->GetObjectSize());
    return result;
}

VMTrivialMethod* MakeGetter(VMSymbol* sig, vector<Variable>& arguments,
                            size_t fieldIndex) {
    VMGetter* result =
        new (GetHeap<HEAP_CLS>(), 0) VMGetter(sig, arguments, fieldIndex);
    LOG_ALLOCATION("VMGetter", result->GetObjectSize());
    return result;
}

VMTrivialMethod* MakeSetter(VMSymbol* sig, vector<Variable>& arguments,
                            size_t fieldIndex, size_t argIndex) {
    VMSetter* result = new (GetHeap<HEAP_CLS>(), 0)
        VMSetter(sig, arguments, fieldIndex, argIndex);
    LOG_ALLOCATION("VMSetter", result->GetObjectSize());
    return result;
}

VMFrame* VMLiteralReturn::Invoke(Interpreter*, VMFrame* frame) {
    for (int i = 0; i < numberOfArguments; i += 1) {
        frame->Pop();
    }
    frame->Push(load_ptr(literal));
    return nullptr;
}

AbstractVMObject* VMLiteralReturn::CloneForMovingGC() const {
    VMLiteralReturn* prim =
        new (GetHeap<HEAP_CLS>(), 0 ALLOC_MATURE) VMLiteralReturn(*this);
    return prim;
}

std::string VMLiteralReturn::AsDebugString() const {
    return "VMLiteralReturn(" + AS_OBJ(load_ptr(literal))->AsDebugString() +
           ")";
}

void VMLiteralReturn::WalkObjects(walk_heap_fn walk) {
    VMInvokable::WalkObjects(walk);
    literal = walk(literal);
}

void VMLiteralReturn::InlineInto(MethodGenerationContext& mgenc, bool) {
    EmitPUSHCONSTANT(mgenc, load_ptr(literal));
}

VMFrame* VMGlobalReturn::Invoke(Interpreter* interpreter, VMFrame* frame) {
    for (int i = 0; i < numberOfArguments; i += 1) {
        frame->Pop();
    }

    vm_oop_t value = GetUniverse()->GetGlobal(load_ptr(globalName));
    if (value != nullptr) {
        frame->Push(value);
    } else {
        interpreter->SendUnknownGlobal(load_ptr(globalName));
    }

    return nullptr;
}

void VMGlobalReturn::InlineInto(MethodGenerationContext& mgenc, bool) {
    EmitPUSHGLOBAL(mgenc, load_ptr(globalName));
}

void VMGlobalReturn::WalkObjects(walk_heap_fn walk) {
    VMInvokable::WalkObjects(walk);
    globalName = (GCSymbol*)walk(globalName);
}

std::string VMGlobalReturn::AsDebugString() const {
    return "VMGlobalReturn(" + AS_OBJ(load_ptr(globalName))->AsDebugString() +
           ")";
}

AbstractVMObject* VMGlobalReturn::CloneForMovingGC() const {
    VMGlobalReturn* prim =
        new (GetHeap<HEAP_CLS>(), 0 ALLOC_MATURE) VMGlobalReturn(*this);
    return prim;
}

VMFrame* VMGetter::Invoke(Interpreter*, VMFrame* frame) {
    vm_oop_t self = nullptr;
    for (int i = 0; i < numberOfArguments; i += 1) {
        self = frame->Pop();
    }

    assert(self != nullptr);

    vm_oop_t result;
    if (unlikely(IS_TAGGED(self))) {
        result = nullptr;
        Universe()->ErrorExit("Integers do not have fields!");
    } else {
        result = ((VMObject*)self)->GetField(fieldIndex);
    }

    frame->Push(result);

    return nullptr;
}

void VMGetter::InlineInto(MethodGenerationContext& mgenc, bool) {
    EmitPushFieldWithIndex(mgenc, fieldIndex);
}

AbstractVMObject* VMGetter::CloneForMovingGC() const {
    VMGetter* prim = new (GetHeap<HEAP_CLS>(), 0 ALLOC_MATURE) VMGetter(*this);
    return prim;
}

void VMGetter::WalkObjects(walk_heap_fn walk) {
    VMInvokable::WalkObjects(walk);
}

std::string VMGetter::AsDebugString() const {
    return "VMGetter(fieldIndex: " + to_string(fieldIndex) + ")";
}

VMFrame* VMSetter::Invoke(Interpreter*, VMFrame* frame) {
    vm_oop_t value = nullptr;
    vm_oop_t self = nullptr;

    for (size_t i = numberOfArguments - 1; i > 0; i -= 1) {
        if (i == argIndex) {
            value = frame->Pop();
        } else {
            frame->Pop();
        }
    }

    self = frame->Top();
    assert(self != nullptr);
    assert(value != nullptr);

    if (unlikely(IS_TAGGED(self))) {
        Universe()->ErrorExit("Integers do not have fields!");
    } else {
        ((VMObject*)self)->SetField(fieldIndex, value);
    }

    return nullptr;
}

void VMSetter::InlineInto(MethodGenerationContext& mgenc, bool) {
    GetUniverse()->ErrorExit(
        "We don't currently support blocks for trivial setters");
}

AbstractVMObject* VMSetter::CloneForMovingGC() const {
    VMSetter* prim = new (GetHeap<HEAP_CLS>(), 0 ALLOC_MATURE) VMSetter(*this);
    return prim;
}

void VMSetter::WalkObjects(walk_heap_fn walk) {
    VMInvokable::WalkObjects(walk);
}

std::string VMSetter::AsDebugString() const {
    return "VMSetter(fieldIndex: " + to_string(fieldIndex) +
           ", argIndex: " + to_string(argIndex) + ")";
}
