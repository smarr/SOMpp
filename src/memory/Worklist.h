//
//  Worklist.h
//  SOM
//
//  Created by Jeroen De Geeter on 11/06/14.
//
//

#ifndef SOM_Worklist_h
#define SOM_Worklist_h

#include <deque>
#include <pthread.h>

#include "../vmobjects/AbstractObject.h"
#include "../vmobjects/ObjectFormats.h"

using namespace std;

class Worklist {
    
public:
    Worklist();
    ~Worklist();
    
    void PushFront(VMOBJECT_PTR);
    VMOBJECT_PTR PopFront();
    void PopBack(Worklist*);
    bool Empty();
    
private:
    pthread_mutex_t lock;
    deque<VMOBJECT_PTR>* work;
    
};

#endif
