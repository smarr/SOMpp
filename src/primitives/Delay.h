#pragma once


#include <vmobjects/ObjectFormats.h>
#include <primitivesCore/PrimitiveContainer.h>

class _Delay: public PrimitiveContainer {
public:
    _Delay();
    
    void Wait(Interpreter*, VMFrame*);
    
};
