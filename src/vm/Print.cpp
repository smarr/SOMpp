#include "Print.h"

#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>

#include "../misc/defs.h"
#include "LogAllocation.h"
#include "Universe.h"

using namespace std;

static mutex output_mutex;

void Print(std::string str) {
    lock_guard<mutex> const lock(output_mutex);
    cout << str << flush;
}

void ErrorPrint(std::string str) {
    lock_guard<mutex> const lock(output_mutex);
    cerr << str << flush;
}

void Print(const char* str) {
    lock_guard<mutex> const lock(output_mutex);
    cout << str << flush;
}

void ErrorPrint(const char* str) {
    lock_guard<mutex> const lock(output_mutex);
    cerr << str << flush;
}

__attribute__((noreturn)) __attribute__((noinline)) void ErrorExit(
    const char* err) {
    ErrorPrint("Runtime error: " + std::string(err) + "\n");
    Quit(ERR_FAIL);
}

__attribute__((noreturn)) __attribute__((noinline)) void Quit(long err) {
    ErrorPrint("Time spent in GC: [" +
               to_string(Timer::GCTimer->GetTotalTime()) + "] msec\n");

    Universe::Shutdown();

    OutputAllocationLogFile();
    exit((int)err);
}
