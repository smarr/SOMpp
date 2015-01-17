//
//  VMThread.cpp
//  SOM
//
//  Created by Jeroen De Geeter on 5/03/14.
//
//

#include "VMThread.h"

#include <sched.h>
#include "../vm/Universe.h"

#include <vmobjects/VMString.h>
#include <vmobjects/VMBlock.h>
#include <vmObjects/VMClass.h>

#include <interpreter/Interpreter.h>

const int VMThread::VMThreadNumberOfFields = 5;

VMThread::VMThread() : VMObject(VMThreadNumberOfFields) {
}

void VMThread::Yield() {
    sched_yield();
}

VMSignal* VMThread::GetResumeSignal() {
    return load_ptr(resumeSignal);
}


void VMThread::SetResumeSignal(VMSignal* value) {
    store_ptr(resumeSignal, value);
}


bool VMThread::ShouldStop() {
    return load_ptr(shouldStop) == load_ptr(trueObject);
}


void VMThread::SetShouldStop(bool value) {
    if (value) {
    	store_ptr(shouldStop, load_ptr(trueObject));
    } else {
    	store_ptr(shouldStop, load_ptr(falseObject));
    }
}


VMBlock* VMThread::GetBlockToRun() {
    return load_ptr(blockToRun);
}


void VMThread::SetBlockToRun(VMBlock* value) {
    store_ptr(blockToRun, value);
}


VMString* VMThread::GetName() {
    return load_ptr(name);
}


void VMThread::SetName(VMString* value) {
    store_ptr(name, value);
}


vm_oop_t VMThread::GetArgument() {
    return load_ptr(argument);
}


void VMThread::SetArgument(vm_oop_t value) {
    store_ptr(argument, value);
}


pthread_t VMThread::GetEmbeddedThreadId() {
	return embeddedThreadId;
}


void VMThread::SetEmbeddedThreadId(pthread_t value) {
    embeddedThreadId = value;
}

void VMThread::Join(int* exitStatus) {
    int returnValue;
	pthread_join(embeddedThreadId, (void**)&returnValue);
    //pthread_join(embeddedThreadId, (void**)&exitStatus);
}

#if GC_TYPE==PAUSELESS
VMThread* VMThread::Clone(Interpreter* thread) {
    VMThread* clone = new (_HEAP, thread, objectSize - sizeof(VMThread)) VMThread(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    return clone;
}
VMThread* VMThread::Clone(PauselessCollectorThread* thread) {
    VMThread* clone = new (_HEAP, thread, objectSize - sizeof(VMThread)) VMThread(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    return clone;
}

void VMThread::MarkReferences() {
    ReadBarrierForGCThread(&clazz);
    ReadBarrierForGCThread(&resumeSignal);
    ReadBarrierForGCThread(&shouldStop);
    ReadBarrierForGCThread(&blockToRun);
    ReadBarrierForGCThread(&name);
    ReadBarrierForGCThread(&argument);
}

void VMThread::CheckMarking(void (*walk)(vm_oop_t)) {
    assert(GetNMTValue(clazz) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(clazz));
    walk(Untag(clazz));
    assert(GetNMTValue(resumeSignal) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(resumeSignal));
    walk(Untag(resumeSignal));
    assert(GetNMTValue(shouldStop) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(shouldStop));
    walk(Untag(shouldStop));
    assert(GetNMTValue(blockToRun) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(blockToRun));
    walk(Untag(blockToRun));
    assert(GetNMTValue(name) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(name));
    walk(Untag(name));
    assert(GetNMTValue(argument) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(argument));
    walk(Untag(argument));
}
#else
/*
void VMThread::WalkObjects(VMOBJECT_PTR (*walk)(VMOBJECT_PTR)) {
    //clazz = (GCClass*) (walk(load_ptr(clazz)));
    //resumeSignal = (GCSignal*) (walk(load_ptr(resumeSignal)));
    //shouldStop = (GCObject*) (walk(load_ptr(shouldStop)));
    //blockToRun = (GCBlock*) (walk(load_ptr(blockToRun)));
    //name = (GCString*) (walk(load_ptr(name)));
    //argument = (GCAbstractObject*) (walk(load_ptr(argument)));
}

VMThread* VMThread::Clone() {
    VMThread* clone = new (_HEAP, _PAGE, objectSize - sizeof(VMThread)) VMThread(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    return clone;
} */
#endif
