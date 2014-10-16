#include "Worklist.h"

#if GC_TYPE == PAUSELESS

Worklist::Worklist() {
    pthread_mutex_init(&lock, NULL);
    work = vector<AbstractVMObject*>();
}

Worklist::~Worklist() {
}

void Worklist::AddWork(AbstractVMObject* reference) {
    pthread_mutex_lock(&lock);
    work.push_back(reference);
    pthread_mutex_unlock(&lock);
}

VMOBJECT_PTR Worklist::GetWork() {
    pthread_mutex_lock(&lock);
    VMOBJECT_PTR reference = work.back();
    work.pop_back();
    pthread_mutex_unlock(&lock);
    return reference;
}

void Worklist::MoveWork(Worklist* moveToWorklist) {
    if (pthread_mutex_trylock(&lock) == 0) {
        while (!work.empty()) {
            moveToWorklist->AddWork(work.back());
            work.pop_back();
        }
        pthread_mutex_unlock(&lock);
    }
}

bool Worklist::Empty() {
    return work.empty();
}

#endif
