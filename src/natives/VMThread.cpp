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

pVMSignal VMThread::GetResumeSignal() {
    return READBARRIER(resumeSignal);
}


void VMThread::SetResumeSignal(pVMSignal value) {
    resumeSignal = WRITEBARRIER(value);
}


bool VMThread::ShouldStop() {
    return READBARRIER(shouldStop) == READBARRIER(trueObject);
}


void VMThread::SetShouldStop(bool value) {
    if (value) {
    	shouldStop = WRITEBARRIER(READBARRIER(trueObject));
    } else {
    	shouldStop = WRITEBARRIER(READBARRIER(falseObject));
    }
}


pVMBlock VMThread::GetBlockToRun() {
    return READBARRIER(blockToRun);
}


void VMThread::SetBlockToRun(pVMBlock value) {
    blockToRun = WRITEBARRIER(value);
}


pVMString VMThread::GetName() {
    return READBARRIER(name);
}


void VMThread::SetName(pVMString value) {
    name = WRITEBARRIER(value);
}


pVMObject VMThread::GetArgument() {
    return READBARRIER(argument);
}


void VMThread::SetArgument(pVMObject value) {
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
pVMThread VMThread::Clone(Interpreter* thread) {
    pVMThread clone = new (_HEAP, thread, objectSize - sizeof(VMThread)) VMThread(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    return clone;
}
pVMThread VMThread::Clone(PauselessCollectorThread* thread) {
    pVMThread clone = new (_HEAP, thread, objectSize - sizeof(VMThread)) VMThread(*this);
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

void VMThread::CheckMarking(void (*walk)(AbstractVMObject*)) {
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
    //clazz = (GCClass*) (walk(READBARRIER(clazz)));
    //resumeSignal = (GCSignal*) (walk(READBARRIER(resumeSignal)));
    //shouldStop = (GCObject*) (walk(READBARRIER(shouldStop)));
    //blockToRun = (GCBlock*) (walk(READBARRIER(blockToRun)));
    //name = (GCString*) (walk(READBARRIER(name)));
    //argument = (GCAbstractObject*) (walk(READBARRIER(argument)));
}

pVMThread VMThread::Clone() {
    pVMThread clone = new (_HEAP, _PAGE, objectSize - sizeof(VMThread)) VMThread(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    return clone;
} */
#endif
