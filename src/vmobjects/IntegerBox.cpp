#include "IntegerBox.h"
#include "VMInteger.h"
#include "../vm/Universe.h"

VMInteger* GlobalBox::integerBox = nullptr;

void GlobalBox::updateIntegerBox(VMInteger* newValue) {
    integerBox = newValue;
}

VMInteger* GlobalBox::IntegerBox() {
    if (integerBox == nullptr) {
        integerBox = GetUniverse()->NewInteger(1);
    }
    return integerBox;
}

