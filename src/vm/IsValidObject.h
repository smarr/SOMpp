#pragma once

#include "../misc/defs.h"
#include "../vmobjects/ObjectFormats.h"

bool IsValidObject(vm_oop_t obj);

bool IsVMInteger(vm_oop_t obj);
bool IsVMMethod(vm_oop_t obj);
bool IsVMSymbol(vm_oop_t obj);
bool IsLiteralReturn(vm_oop_t obj);
bool IsGlobalReturn(vm_oop_t obj);
bool IsGetter(vm_oop_t obj);
bool IsSetter(vm_oop_t obj);
bool IsSafeUnaryPrim(vm_oop_t obj);

void set_vt_to_null();

void obtain_vtables_of_known_classes(VMSymbol* someValidSymbol);
