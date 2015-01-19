#include "BaseThread.h"

#if GC_TYPE==PAUSELESS
    #include "PauselessHeap.h"
#elif GC_TYPE==GENERATIONAL
    #include "GenerationalHeap.h"
#endif

//#include "GenerationalHeap.h"


#if GC_TYPE==PAUSELESS
BaseThread::BaseThread() {
    page = _HEAP->RequestPage();
    worklist = Worklist();
    this->expectedNMT = false;
}

BaseThread::BaseThread(bool expectedNMT) {
    page = _HEAP->RequestPage();
    worklist = Worklist();
    this->expectedNMT = expectedNMT;
}
#else
BaseThread::BaseThread() {
    page = _HEAP->RequestPage();
}
#endif

BaseThread::~BaseThread() {
   _HEAP->RelinquishPage(page);
}

Page* BaseThread::GetPage() {
    return this->page;
}

void BaseThread::SetPage(Page* page) {
    this->page = page;
}

#if GC_TYPE==PAUSELESS
bool BaseThread::GetExpectedNMT() {
    return expectedNMT;
}
#endif
