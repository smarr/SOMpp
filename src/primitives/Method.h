#pragma once

#include <vmobjects/ObjectFormats.h>
#include <primitivesCore/PrimitiveContainer.h>

class _Method: public PrimitiveContainer {
public:
    _Method(void);
    
    void Signature(Interpreter*, VMFrame*);
    void Holder   (Interpreter*, VMFrame*);
    void InvokeOn_With_(Interpreter*, VMFrame*);

};
