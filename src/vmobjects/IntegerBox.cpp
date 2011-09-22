#include "IntegerBox.h"
#include "VMInteger.h"
#include "../vm/Universe.h"

VMInteger* GlobalBox::integerBox = NULL;

void GlobalBox::updateIntegerBox(VMInteger* newValue) {
    integerBox = newValue;
}

VMInteger* GlobalBox::IntegerBox()
{
    if (integerBox == NULL) {
        integerBox = _UNIVERSE->NewInteger(1);
    }
    return integerBox;
}

