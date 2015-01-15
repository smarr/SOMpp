#pragma once

class VMObject;
class VMFrame;

#include "../primitivesCore/PrimitiveContainer.h"

class _Primitive: public PrimitiveContainer {
public:
    _Primitive(void);
    
    void Signature(pVMObject object, pVMFrame frame);
    void Holder   (pVMObject object, pVMFrame frame);
};
