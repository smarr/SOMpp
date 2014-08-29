#pragma once

#include <vmobjects/ObjectFormats.h>

class GlobalBox {
public:
    static VMInteger* IntegerBox();

private:
    static void updateIntegerBox(VMInteger*);
    static VMInteger* integerBox;
    friend class Universe;
};
