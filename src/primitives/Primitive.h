#pragma once

#include "../primitivesCore/PrimitiveContainer.h"
#include "../vmobjects/ObjectFormats.h"

class _Primitive: public PrimitiveContainer {
public:
    _Primitive(void);
    
    void Signature(Interpreter*, VMFrame*);
    void Holder   (Interpreter*, VMFrame*);
    void InvokeOn_With_(Interpreter*, VMFrame*);

};
