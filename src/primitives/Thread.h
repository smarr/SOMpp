#pragma once


#include <vmobjects/ObjectFormats.h>
#include <primitivesCore/PrimitiveContainer.h>

class _Thread: public PrimitiveContainer {
public:
    _Thread();
    void Name(Interpreter*, VMFrame*);
    void Name_(Interpreter*, VMFrame*);
    void Join(Interpreter*, VMFrame*);
    
    void Yield(Interpreter*, VMFrame*);
    void Current(Interpreter*, VMFrame*);
};
