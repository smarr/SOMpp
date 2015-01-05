#include "BaseThread.h"

#if GC_TYPE==PAUSELESS
    #include "pauseless/PauselessHeap.h"
#endif

#if GC_TYPE!=PAUSELESS
BaseThread::BaseThread() {
    page = _HEAP->RequestPage();
}
#else
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
#endif

BaseThread::~BaseThread() {
   _HEAP->RelinquishPage(page);
}

Page* BaseThread::GetPage() {
    return page;
}

void BaseThread::SetPage(Page* page) {
    this->page = page;
}

#if GC_TYPE==PAUSELESS
bool BaseThread::GetExpectedNMT() {
    return expectedNMT;
}
#endif
