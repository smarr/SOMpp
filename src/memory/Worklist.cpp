//
//  Worklist.cpp
//  SOM
//
//  Created by Jeroen De Geeter on 11/06/14.
//
//

#include "Worklist.h"

Worklist::Worklist() {
    pthread_mutex_init(&lock, NULL);
    work = new deque<VMOBJECT_PTR>();
}

Worklist::~Worklist() {
    delete work;
}

void Worklist::PushFront(VMOBJECT_PTR reference) {
    work->push_front(reference);
}

VMOBJECT_PTR Worklist::PopFront() {
    VMOBJECT_PTR reference = work->front();
    work->pop_front();
    return reference;
}

void Worklist::PopBack(Worklist* worklist) {
    if (pthread_mutex_trylock(&lock) == 0) {
        worklist->PushFront(work->back());
        work->pop_back();
        pthread_mutex_unlock(&lock);
    }
}

bool Worklist::Empty() {
    return work->empty();
}