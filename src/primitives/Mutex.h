#pragma once


#include <vmobjects/ObjectFormats.h>
#include <primitivesCore/PrimitiveContainer.h>

class _Mutex: public PrimitiveContainer {
public:
    _Mutex();

    void Lock(Interpreter*, VMFrame*);
    void Unlock(Interpreter*, VMFrame*);
    void IsLocked(Interpreter*, VMFrame*);
    void NewCondition(Interpreter*, VMFrame*);
    void New(Interpreter*, VMFrame*);
    
};
