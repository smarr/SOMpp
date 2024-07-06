#include <vmobjects/ObjectFormats.h>

#if !DEBUG
void set_vt_to_null() {}

void obtain_vtables_of_known_classes(VMSymbol* className) {}

inline bool IsValidObject(vm_oop_t obj) {
    return true;
}
#else
bool IsValidObject(vm_oop_t obj);
void set_vt_to_null();
void obtain_vtables_of_known_classes(VMSymbol* className);
#endif
