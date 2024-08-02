#pragma once

#include "../primitivesCore/PrimitiveContainer.h"
#include "../vmobjects/ObjectFormats.h"

class _Method : public PrimitiveContainer {
public:
    _Method(void);

    void InvokeOn_With_(Interpreter*, VMFrame*);
};
