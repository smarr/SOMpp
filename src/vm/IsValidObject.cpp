#include <cassert>

#include "../memory/Heap.h"
#include "../misc/defs.h"
#include "../vmobjects/AbstractObject.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMArray.h"
#include "../vmobjects/VMBlock.h"
#include "../vmobjects/VMDouble.h"
#include "../vmobjects/VMEvaluationPrimitive.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMInteger.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMString.h"
#include "../vmobjects/VMSymbol.h"
#include "Globals.h"
#include "IsValidObject.h"

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
void* vt_string;
void* vt_symbol;

bool IsValidObject(vm_oop_t obj) {
    if (DEBUG) {
        return true;
    }
    
    if (IS_TAGGED(obj))
        return true;

    if (obj == INVALID_VM_POINTER
        // || obj == nullptr
        ) {
        assert(false);
        return false;
    }
    
    if (obj == nullptr)
        return true;
    
    
    if (vt_symbol == nullptr) // initialization not yet completed
        return true;
    
    void* vt = *(void**) obj;
    bool b = vt == vt_array    ||
           vt == vt_block      ||
           vt == vt_class      ||
           vt == vt_double     ||
           vt == vt_eval_primitive ||
           vt == vt_frame      ||
           vt == vt_integer    ||
           vt == vt_method     ||
           vt == vt_object     ||
           vt == vt_primitive  ||
           vt == vt_string     ||
           vt == vt_symbol;
    assert(b);
    return b;
}

void set_vt_to_null() {
    vt_array      = nullptr;
    vt_block      = nullptr;
    vt_class      = nullptr;
    vt_double     = nullptr;
    vt_eval_primitive = nullptr;
    vt_frame      = nullptr;
    vt_integer    = nullptr;
    vt_method     = nullptr;
    vt_object     = nullptr;
    vt_primitive  = nullptr;
    vt_string     = nullptr;
    vt_symbol     = nullptr;
}

void obtain_vtables_of_known_classes(VMSymbol* className) {
    VMArray* arr  = new (GetHeap<HEAP_CLS>()) VMArray(0, 0);
    vt_array      = *(void**) arr;
    
    VMBlock* blck = new (GetHeap<HEAP_CLS>()) VMBlock();
    vt_block      = *(void**) blck;
    
    vt_class      = *(void**) symbolClass;
    
    VMDouble* dbl = new (GetHeap<HEAP_CLS>()) VMDouble(0.0);
    vt_double     = *(void**) dbl;
    
    VMEvaluationPrimitive* ev = new (GetHeap<HEAP_CLS>()) VMEvaluationPrimitive(1);
    vt_eval_primitive = *(void**) ev;
    
    VMFrame* frm  = new (GetHeap<HEAP_CLS>()) VMFrame(0, 0);
    vt_frame      = *(void**) frm;
    
    VMInteger* i  = new (GetHeap<HEAP_CLS>()) VMInteger(0);
    vt_integer    = *(void**) i;
    
    VMMethod* mth = new (GetHeap<HEAP_CLS>()) VMMethod(nullptr, 0, 0, 0, 0);
    vt_method     = *(void**) mth;
    vt_object     = *(void**) nilObject;
    
    VMPrimitive* prm = new (GetHeap<HEAP_CLS>()) VMPrimitive(className);
    vt_primitive  = *(void**) prm;
    
    VMString* str = new (GetHeap<HEAP_CLS>(), PADDED_SIZE(1)) VMString(0, nullptr);
    vt_string     = *(void**) str;
    vt_symbol     = *(void**) className;
}
