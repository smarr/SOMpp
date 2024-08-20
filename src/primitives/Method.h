#pragma once

#include "../primitivesCore/PrimitiveContainer.h"
#include "../vmobjects/ObjectFormats.h"

class _Method : public PrimitiveContainer {
public:
    _Method();

    void InvokeOn_With_(VMFrame*);
};
