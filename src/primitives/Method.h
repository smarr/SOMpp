#pragma once

#include <vmobjects/ObjectFormats.h>
#include <primitivesCore/PrimitiveContainer.h>

class _Method: public PrimitiveContainer {
public:
    _Method(void);
    
    void Signature(pVMObject object, pVMFrame frame);
    void Holder   (pVMObject object, pVMFrame frame);
};
