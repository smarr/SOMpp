#pragma once

#include <vmobjects/ObjectFormats.h>
#include <primitivesCore/PrimitiveContainer.h>

class _Method: public PrimitiveContainer {
public:
    _Method(void);
    
    void Signature(VMObject* object, VMFrame* frame);
    void Holder   (VMObject* object, VMFrame* frame);
};
