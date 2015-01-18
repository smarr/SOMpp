#include "Worklist.h"
#include "PauselessCollectorThread.h"
#include <assert.h>

#if GC_TYPE == PAUSELESS

Worklist::Worklist() {
    pthread_mutex_init(&lock, nullptr);
    work = vector<AbstractVMObject*>();
}

Worklist::~Worklist() {
}

void Worklist::AddWorkGC(AbstractVMObject* reference) {
    pthread_mutex_lock(&lock);
    work.push_back(reference);
    pthread_mutex_unlock(&lock);
}

void Worklist::AddWorkMutator(AbstractVMObject* reference) {
    pthread_mutex_lock(&lock);
    bool wasEmpty = work.empty();
    work.push_back(reference);
    pthread_mutex_unlock(&lock);
    if (wasEmpty) {
        PauselessCollectorThread::AddNonEmptyWorklist(this);
    }
}

AbstractVMObject* Worklist::GetWork() {
    pthread_mutex_lock(&lock);
    AbstractVMObject* reference = work.back();
    work.pop_back();
    pthread_mutex_unlock(&lock);
    return reference;
}

void Worklist::MoveWork(Worklist* moveToWorklist) {
    pthread_mutex_lock(&lock);
    while (!work.empty()) {
        moveToWorklist->AddWorkGC(work.back());
        work.pop_back();
    }
    pthread_mutex_unlock(&lock);
}

bool Worklist::Empty() {
    return work.empty();
}

void Worklist::Clear() {
    pthread_mutex_lock(&lock);
    work.clear();
    pthread_mutex_unlock(&lock);
}

#endif
