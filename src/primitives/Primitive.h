#ifndef CORE_PRIMITIVE_H_
#define CORE_PRIMITIVE_H_

class VMObject;
class VMFrame;

#include "../primitivesCore/PrimitiveContainer.h"

class _Primitive: public PrimitiveContainer {
public:
    _Primitive(void);
    
    void Signature(pVMObject object, pVMFrame frame);
    void Holder   (pVMObject object, pVMFrame frame);
};

#endif /* defined(CORE_METHOD_H_) */
