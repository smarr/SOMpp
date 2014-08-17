#pragma once

#ifndef INTEGERBOX_H_
#define INTEGERBOX_H_

#include <vmobjects/ObjectFormats.h>

class GlobalBox {
public:
    static VMInteger* IntegerBox();

private:
    static void updateIntegerBox(VMInteger*);
    static VMInteger* integerBox;
    friend class Universe;
};

#endif
