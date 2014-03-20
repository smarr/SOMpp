//
//  Delay.h
//  SOM
//
//  Created by Jeroen De Geeter on 10/11/13.
//
//

#ifndef SOM_Delay_h
#define SOM_Delay_h

#include "../primitivesCore/PrimitiveContainer.h"

class _Delay : public PrimitiveContainer
{
    
public:
    
    _Delay();
    
    /** makes the current thread wait for a certain amount of time */
    void Wait(pVMObject object, pVMFrame frame);
    
};

#endif
