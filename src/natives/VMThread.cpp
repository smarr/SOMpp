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


ThreadId VMThread::GetEmbeddedThreadId() {
	return embeddedThreadId;
}


void VMThread::SetEmbeddedThreadId(ThreadId value) {
    embeddedThreadId = value;
}

/*
int VMThread::GetThreadId() {
    return threadId;
}

void VMThread::SetThreadId(int value) {
    threadId = value;
} */

void VMThread::Join(int* exitStatus) {
    int returnValue;
	pthread_join(embeddedThreadId, (void**)&returnValue);
}