#pragma once

#include <string>

#include "../vmobjects/ObjectFormats.h"
#include "defs.h"

vm_oop_t ParseInteger(const char* str, int base, bool negateValue);
vm_oop_t ParseInteger(std::string& str, int base, bool negateValue);
