#pragma once
//
//  Signal.h
//  SOM
//
//  Created by Jeroen De Geeter on 9/11/13.
//
//

#include "../primitivesCore/PrimitiveContainer.h"

class _Signal : public PrimitiveContainer
{
    
public:
    
    _Signal();
    
    /** Blocking wait for this signal */
    void Wait(Interpreter*, VMFrame*);
    /** Signal one waiting Thread */
    void Signal(Interpreter*, VMFrame*);
    /** Wakes up all the Threads waiting for the signal */
    void SignalAll(Interpreter*, VMFrame*);
    
    /** creates a new Signal Object with encapsulated pthread signal */
    void New(Interpreter*, VMFrame*);
    
};
