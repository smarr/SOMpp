#include "BaseThread.h"

#if GC_TYPE==PAUSELESS
    #include "pauseless/PauselessHeap.h"
#endif

BaseThread::BaseThread() {
    page = _HEAP->RequestPage();
#if GC_TYPE==PAUSELESS
    worklist = Worklist();
    expectedNMT = false;
#endif
}

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
