//
//  Mutex.h
//  SOM
//
//  Created by Jeroen De Geeter on 10/11/13.
//
//

#ifndef SOM_Mutex_h
#define SOM_Mutex_h

class VMObject;
class VMFrame;

#include "../primitivesCore/PrimitiveContainer.h"

class _Mutex: public PrimitiveContainer {

public:
    
    _Mutex();

    //Locks the access to the encapsulated Mutex
    void Lock(pVMObject object, pVMFrame frame);
    // Frees the mutex
    void Unlock(pVMObject object, pVMFrame frame);
    /* Returns true if the mutex is free, false otherwise
     * Enables nonblocking mutex checks
     */
    void IsLocked(pVMObject object, pVMFrame frame);

    void New(pVMObject object, pVMFrame frame);
    
};

#endif
