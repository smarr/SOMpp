#pragma once


#include <vmobjects/ObjectFormats.h>
#include <primitivesCore/PrimitiveContainer.h>

class _Condition: public PrimitiveContainer {
public:
    _Condition();
    void SignalOne(Interpreter*, VMFrame*);
    void SignalAll(Interpreter*, VMFrame*);
    
    void Await(Interpreter*, VMFrame*);
    void Await_(Interpreter*, VMFrame*);

};
