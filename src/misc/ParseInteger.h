#pragma once

#include <misc/defs.h>
#include <vmobjects/ObjectFormats.h>

vm_oop_t ParseInteger(const char* str, int base, bool negateValue);
vm_oop_t ParseInteger(StdString& str, int base, bool negateValue);
