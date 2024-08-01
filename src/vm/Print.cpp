#include "Print.h"

#include <iostream>
#include <mutex>

#include "../misc/defs.h"

using namespace std;

static mutex output_mutex;

void Print(StdString str) {
    lock_guard<mutex> lock(output_mutex);
    cout << str << flush;
}

void ErrorPrint(StdString str) {
    lock_guard<mutex> lock(output_mutex);
    cerr << str << flush;
}
