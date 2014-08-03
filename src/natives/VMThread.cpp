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

const int VMThread::VMThreadNumberOfFields = 5;

VMThread::VMThread() : VMObject(VMThreadNumberOfFields) {
}

void VMThread::Yield() {
    sched_yield();
}

pVMSignal VMThread::GetResumeSignal() {
    PG_HEAP(ReadBarrier((void**)(&resumeSignal)));
    return resumeSignal;
}


void VMThread::SetResumeSignal(pVMSignal value) {
    resumeSignal = value;
}


bool VMThread::ShouldStop() {
    PG_HEAP(ReadBarrier((void**)(&shouldStop)));
    return shouldStop == trueObject;
}


void VMThread::SetShouldStop(bool value) {
    if (value) {
        PG_HEAP(ReadBarrier((void**)(&trueObject)));
    	shouldStop = trueObject;
    } else {
        PG_HEAP(ReadBarrier((void**)(&falseObject)));
    	shouldStop = falseObject;
    }
}


pVMBlock VMThread::GetBlockToRun() {
    PG_HEAP(ReadBarrier((void**)(&blockToRun)));
    return blockToRun;
}


void VMThread::SetBlockToRun(pVMBlock value) {
    blockToRun = value;
}


pVMString VMThread::GetName() {
    PG_HEAP(ReadBarrier((void**)(&name)));
    return name;
}


void VMThread::SetName(pVMString value) {
    name = value;
}


pVMObject VMThread::GetArgument() {
    PG_HEAP(ReadBarrier((void**)(&argument)));
    return argument;
}


void VMThread::SetArgument(pVMObject value) {
    argument = value;
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
	pthread_join(embeddedThreadId, (void**)exitStatus);
}