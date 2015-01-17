#pragma once
//
//  Mutex.h
//  SOM
//
//  Created by Jeroen De Geeter on 10/11/13.
//
//


#include <vmobjects/ObjectFormats.h>
#include <primitivesCore/PrimitiveContainer.h>

class _Mutex: public PrimitiveContainer {
public:
    
    _Mutex();

    //Locks the access to the encapsulated Mutex
    void Lock(Interpreter*, VMFrame*);
    // Frees the mutex
    void Unlock(Interpreter*, VMFrame*);
    /* Returns true if the mutex is free, false otherwise
     * Enables nonblocking mutex checks
     */
    void IsLocked(Interpreter*, VMFrame*);

    void New(Interpreter*, VMFrame*);
    
};
