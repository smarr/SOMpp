#pragma once

#include "../primitivesCore/PrimitiveContainer.h"
#include "../vmobjects/ObjectFormats.h"

class _Primitive : public PrimitiveContainer {
public:
    _Primitive(void);

    void InvokeOn_With_(Interpreter*, VMFrame*);
};
