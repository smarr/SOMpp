#pragma once

//
//  Thread.h
//  SOM
//
//  Created by Jeroen De Geeter on 31/10/13.
//
//

#include <primitivesCore/PrimitiveContainer.h>

class _Thread : public PrimitiveContainer {
public:
    _Thread();
    
    /** Waites for the Thread to finish an destroys it*/
    void Join(Interpreter*, VMFrame*);
    /** Sets the scheduling priority of the embedded thread*/
    void Priority_(Interpreter*, VMFrame*);

    //how do I make a difference between class methods and object methods? Can I simply do this by adding these to the class side in the thread.som file?
    /** Releases the processing time of the Objects pthread for some time */
    void Yield(Interpreter*, VMFrame*);
    /** Returns the current Thread object running */
    void Current(Interpreter*, VMFrame*);
};
