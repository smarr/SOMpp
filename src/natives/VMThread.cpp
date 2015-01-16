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
    resumeSignal = WRITEBARRIER(value);
}


bool VMThread::ShouldStop() {
    return load_ptr(shouldStop) == load_ptr(trueObject);
}


void VMThread::SetShouldStop(bool value) {
    if (value) {
    	shouldStop = WRITEBARRIER(load_ptr(trueObject));
    } else {
    	shouldStop = WRITEBARRIER(load_ptr(falseObject));
    }
}


VMBlock* VMThread::GetBlockToRun() {
    return load_ptr(blockToRun);
}


void VMThread::SetBlockToRun(VMBlock* value) {
    blockToRun = WRITEBARRIER(value);
}


VMString* VMThread::GetName() {
    return load_ptr(name);
}


void VMThread::SetName(VMString* value) {
    name = WRITEBARRIER(value);
}


vm_oop_t VMThread::GetArgument() {
    return load_ptr(argument);
}


void VMThread::SetArgument(vm_oop_t value) {
    argument = WRITEBARRIER(value);
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
