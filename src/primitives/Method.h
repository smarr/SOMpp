#pragma once

#include <vmobjects/ObjectFormats.h>
#include <primitivesCore/PrimitiveContainer.h>

class _Method: public PrimitiveContainer {
public:
    _Method(void);
    
    void Signature(pVMObject object, VMFrame* frame);
    void Holder   (pVMObject object, VMFrame* frame);
};
