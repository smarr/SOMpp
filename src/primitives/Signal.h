//
//  Signal.h
//  SOM
//
//  Created by Jeroen De Geeter on 9/11/13.
//
//

#ifndef SOM_Signal_h
#define SOM_Signal_h

#include "../primitivesCore/PrimitiveContainer.h"

class _Signal : public PrimitiveContainer
{
    
public:
    
    _Signal();
    
    /** Blocking wait for this signal */
    void Wait(pVMObject object, pVMFrame frame);
    /** Signal one waiting Thread */
    void Signal(pVMObject object, pVMFrame frame);
    /** Wakes up all the Threads waiting for the signal */
    void SignalAll(pVMObject object, pVMFrame frame);
    
    /** creates a new Signal Object with encapsulated pthread signal */
    void New(pVMObject object, pVMFrame frame);
    
};

#endif
