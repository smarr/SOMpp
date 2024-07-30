#include <string>

#include "../compiler/Disassembler.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMSymbol.h"
#include "debug.h"


std::string DebugGetClassName(vm_oop_t obj) {
    return CLASS_OF(obj)->GetName()->GetStdString();
}

std::string DebugGetClassName(gc_oop_t obj) {
    return CLASS_OF(obj)->GetName()->GetStdString();
}

void DebugDumpMethod(VMMethod* method) {
    Disassembler::DumpMethod(method, "", false);
}
