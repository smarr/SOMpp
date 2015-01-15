#pragma once

class VMObject;
class VMFrame;

#include "../primitivesCore/PrimitiveContainer.h"

class _Method: public PrimitiveContainer {
public:
    _Method(void);
    
    void Signature(VMObject* object, VMFrame* frame);
    void Holder   (VMObject* object, VMFrame* frame);
};
