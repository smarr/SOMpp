#pragma once

class VMObject;
class VMFrame;

#include "../primitivesCore/PrimitiveContainer.h"

class _Primitive: public PrimitiveContainer {
public:
    _Primitive(void);
    
    void Signature(VMObject* object, VMFrame* frame);
    void Holder   (VMObject* object, VMFrame* frame);
};
