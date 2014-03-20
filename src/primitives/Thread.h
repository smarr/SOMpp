//
//  Thread.h
//  SOM
//
//  Created by Jeroen De Geeter on 31/10/13.
//
//

#ifndef SOM_Thread_h
#define SOM_Thread_h

#include "../primitivesCore/PrimitiveContainer.h"

class _Thread : public PrimitiveContainer
{

public:

    _Thread();
    
    /** Waites for the Thread to finish an destroys it*/
    void Join(pVMObject object, pVMFrame frame);
    /** Sets the scheduling priority of the embedded thread*/
    void Priority_(pVMObject object, pVMFrame frame);
    
    //class methods
    //how do I make a difference between class methods and object methods? Can I simply do this by adding these to the class side in the thread.som file?
    /** Releases the processing time of the Objects pthread for some time */
    void Yield(pVMObject object, pVMFrame frame);
    /** Returns the current Thread object running */
    void Current(pVMObject object, pVMFrame frame);

};

#endif
