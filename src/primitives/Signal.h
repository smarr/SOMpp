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
    void Wait(VMObject* object, VMFrame* frame);
    /** Signal one waiting Thread */
    void Signal(VMObject* object, VMFrame* frame);
    /** Wakes up all the Threads waiting for the signal */
    void SignalAll(VMObject* object, VMFrame* frame);
    
    /** creates a new Signal Object with encapsulated pthread signal */
    void New(VMObject* object, VMFrame* frame);
    
};
