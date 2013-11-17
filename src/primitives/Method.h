#ifndef CORE_METHOD_H_
#define CORE_METHOD_H_

class VMObject;
class VMFrame;

#include "../primitivesCore/PrimitiveContainer.h"

class _Method: public PrimitiveContainer {
public:
    _Method(void);
    
    void Signature(pVMObject object, pVMFrame frame);
    void Holder   (pVMObject object, pVMFrame frame);
};

#endif /* defined(CORE_METHOD_H_) */
