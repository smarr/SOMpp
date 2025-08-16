#pragma once

#include <string>

#include "../vmobjects/ObjectFormats.h"
#include "defs.h"

vm_oop_t ParseInteger(const char* str, bool negateValue);
vm_oop_t ParseInteger(std::string& str, bool negateValue);
