#include "IsValidObject.h"

#include <cassert>
#include <vector>

#include "../compiler/Variable.h"
#include "../memory/Heap.h"
#include "../misc/defs.h"
#include "../vmobjects/AbstractObject.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMArray.h"
#include "../vmobjects/VMBlock.h"
#include "../vmobjects/VMClass.h"  // NOLINT(misc-include-cleaner) it's required to make the types complete
#include "../vmobjects/VMDouble.h"
#include "../vmobjects/VMEvaluationPrimitive.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMInteger.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMObjectBase.h"  // NOLINT(misc-include-cleaner) needed for some GCs
#include "../vmobjects/VMSafePrimitive.h"
#include "../vmobjects/VMString.h"
#include "../vmobjects/VMSymbol.h"
#include "../vmobjects/VMTrivialMethod.h"
#include "Globals.h"

void* vt_array;
void* vt_block;
void* vt_class;
void* vt_double;
void* vt_eval_primitive;
void* vt_frame;
void* vt_integer;
void* vt_method;
void* vt_object;
void* vt_primitive;
void* vt_safe_un_primitive;
void* vt_safe_bin_primitive;
void* vt_safe_ter_primitive;
void* vt_literal_return;
void* vt_global_return;
void* vt_getter;
void* vt_setter;
void* vt_string;
void* vt_symbol;

bool IsValidObject(vm_oop_t obj) {
    if (!DEBUG) {
        return true;
    }

    if (obj == nullptr) {
        return true;
    }

    if (IS_TAGGED(obj)) {
        return true;
    }

    if (vt_symbol == nullptr) {  // initialization not yet completed
        return true;
    }

    if (obj == INVALID_VM_POINTER) {
        assert(obj == INVALID_VM_POINTER &&
               "Expected pointer to not be marker for invalid pointers.");
        return false;
    }

    void* vt = *(void**)obj;
    bool b = vt == vt_array || vt == vt_block || vt == vt_class ||
             vt == vt_double || vt == vt_eval_primitive || vt == vt_frame ||
             vt == vt_integer || vt == vt_method || vt == vt_object ||
             vt == vt_primitive || vt == vt_safe_un_primitive ||
             vt == vt_safe_bin_primitive || vt == vt_safe_ter_primitive ||
             vt == vt_string || vt == vt_symbol || vt == vt_literal_return ||
             vt == vt_global_return || vt == vt_getter || vt == vt_setter;
    if (!b) {
        assert(b && "Expected vtable to be one of the known ones.");
        return false;
    }

#if GC_TYPE == COPYING || GC_TYPE == DEBUG_COPYING
    if (AS_OBJ(obj)->GetGCField() != 0) {
        // this is a properly forwarded object
        return true;
    }
#elif GC_TYPE == GENERATIONAL
    if (!(AS_OBJ(obj)->GetGCField() & MASK_OBJECT_IS_OLD)) {
        if (AS_OBJ(obj)->GetGCField() != 0) {
            // this is a properly forwarded object
            return true;
        }
    }
#endif

    b = !AS_OBJ(obj)->IsMarkedInvalid();
    assert(b && "Expected object not to be marked as invalid.");
    return b;
}

void set_vt_to_null() {
    vt_array = nullptr;
    vt_block = nullptr;
    vt_class = nullptr;
    vt_double = nullptr;
    vt_eval_primitive = nullptr;
    vt_frame = nullptr;
    vt_integer = nullptr;
    vt_method = nullptr;
    vt_object = nullptr;
    vt_primitive = nullptr;
    vt_safe_un_primitive = nullptr;
    vt_safe_bin_primitive = nullptr;
    vt_safe_ter_primitive = nullptr;
    vt_literal_return = nullptr;
    vt_global_return = nullptr;
    vt_getter = nullptr;
    vt_setter = nullptr;
    vt_string = nullptr;
    vt_symbol = nullptr;
}

static void* get_vtable(AbstractVMObject* obj) {
    return *(void**)obj;
}

bool IsVMInteger(vm_oop_t obj) {
    assert(vt_integer != nullptr);
    return get_vtable(AS_OBJ(obj)) == vt_integer;
}

bool IsVMDouble(vm_oop_t obj) {
    assert(vt_double != nullptr);
    return get_vtable(AS_OBJ(obj)) == vt_double;
}

bool IsVMMethod(vm_oop_t obj) {
    assert(vt_method != nullptr);
    return get_vtable(AS_OBJ(obj)) == vt_method;
}

bool IsVMSymbol(vm_oop_t obj) {
    assert(vt_symbol != nullptr);
    return get_vtable(AS_OBJ(obj)) == vt_symbol;
}

bool IsLiteralReturn(vm_oop_t obj) {
    assert(vt_literal_return != nullptr);
    return get_vtable(AS_OBJ(obj)) == vt_literal_return;
}

bool IsGlobalReturn(vm_oop_t obj) {
    assert(vt_global_return != nullptr);
    return get_vtable(AS_OBJ(obj)) == vt_global_return;
}

bool IsGetter(vm_oop_t obj) {
    assert(vt_getter != nullptr);
    return get_vtable(AS_OBJ(obj)) == vt_getter;
}

bool IsSetter(vm_oop_t obj) {
    assert(vt_setter != nullptr);
    return get_vtable(AS_OBJ(obj)) == vt_setter;
}

void obtain_vtables_of_known_classes(VMSymbol* someValidSymbol) {
    // These objects are allocated on the heap. So, they will get GC'ed soon
    // enough.
    VMArray* arr = new (GetHeap<HEAP_CLS>(), 0) VMArray(0, 0);
    vt_array = get_vtable(arr);

    VMBlock* blck = new (GetHeap<HEAP_CLS>(), 0) VMBlock(nullptr, nullptr);
    vt_block = get_vtable(blck);

    vt_class = get_vtable(load_ptr(symbolClass));

    VMDouble* dbl = new (GetHeap<HEAP_CLS>(), 0) VMDouble(0.0);
    vt_double = get_vtable(dbl);

    VMEvaluationPrimitive* ev =
        new (GetHeap<HEAP_CLS>(), 0) VMEvaluationPrimitive(1);
    vt_eval_primitive = get_vtable(ev);

    VMFrame* frm = new (GetHeap<HEAP_CLS>(), 0) VMFrame(0, 0);
    vt_frame = get_vtable(frm);

    VMInteger* i = new (GetHeap<HEAP_CLS>(), 0) VMInteger(0);
    vt_integer = get_vtable(i);

    VMMethod* mth = new (GetHeap<HEAP_CLS>(), 0)
        VMMethod(nullptr, 0, 0, 0, 0, nullptr, nullptr);
    vt_method = get_vtable(mth);
    vt_object = get_vtable(load_ptr(nilObject));

    VMPrimitive* prm =
        new (GetHeap<HEAP_CLS>(), 0) VMPrimitive(someValidSymbol);
    vt_primitive = get_vtable(prm);

    VMSafeUnaryPrimitive* sbp1 = new (GetHeap<HEAP_CLS>(), 0)
        VMSafeUnaryPrimitive(someValidSymbol, UnaryPrim());
    vt_safe_un_primitive = get_vtable(sbp1);

    VMSafeBinaryPrimitive* sbp2 = new (GetHeap<HEAP_CLS>(), 0)
        VMSafeBinaryPrimitive(someValidSymbol, BinaryPrim());
    vt_safe_bin_primitive = get_vtable(sbp2);

    VMSafeTernaryPrimitive* sbp3 = new (GetHeap<HEAP_CLS>(), 0)
        VMSafeTernaryPrimitive(someValidSymbol, TernaryPrim());
    vt_safe_ter_primitive = get_vtable(sbp3);

    vector<Variable> v;
    VMLiteralReturn* lr = new (GetHeap<HEAP_CLS>(), 0)
        VMLiteralReturn(someValidSymbol, v, someValidSymbol);
    vt_literal_return = get_vtable(lr);

    VMGlobalReturn* gr = new (GetHeap<HEAP_CLS>(), 0)
        VMGlobalReturn(someValidSymbol, v, someValidSymbol);
    vt_global_return = get_vtable(gr);

    VMGetter* get =
        new (GetHeap<HEAP_CLS>(), 0) VMGetter(someValidSymbol, v, 0);
    vt_getter = get_vtable(get);

    VMSetter* set =
        new (GetHeap<HEAP_CLS>(), 0) VMSetter(someValidSymbol, v, 0, 0);
    vt_setter = get_vtable(set);

    VMString* str =
        new (GetHeap<HEAP_CLS>(), PADDED_SIZE(1)) VMString(0, nullptr);
    vt_string = get_vtable(str);
    vt_symbol = get_vtable(someValidSymbol);
}
