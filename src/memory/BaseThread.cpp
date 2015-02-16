#include "BaseThread.h"

#if GC_TYPE==PAUSELESS
    #include "PauselessHeap.h"
#elif GC_TYPE==GENERATIONAL
    #include "GenerationalHeap.h"
#endif

//#include "GenerationalHeap.h"


#if GC_TYPE==PAUSELESS
BaseThread::BaseThread(Page* page, bool gcTrapEnabled)
    : page(page), expectedNMT(false), gcTrapEnabled(gcTrapEnabled)
{
    if (page)
        page->SetNonRelocatablePage(GetHeap<HEAP_CLS>()->pagedHeap.GetNextPage());

}

BaseThread::BaseThread(Page* page, bool expectedNMT, bool gcTrapEnabled)
    : page(page), expectedNMT(expectedNMT), gcTrapEnabled(gcTrapEnabled)
{
    if (page)
        page->SetNonRelocatablePage(GetHeap<HEAP_CLS>()->pagedHeap.GetNextPage());
}
#else
BaseThread::BaseThread(Page* page) : page(page) {}
#endif

BaseThread::~BaseThread() {
#warning TODO: is the page properly reglingushed even if not done here?
//   GetHeap<HEAP_CLS>()->RelinquishPage(page);
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

bool BaseThread::GCTrapEnabled() {
    return gcTrapEnabled;
}

/// The GC trap is to make sure that a page is blocked atomically, and
//  not observable by a mutator. The mutator are not allowed to see the
//  blocking change within the period of two safepoints, because otherwise
//  they could be seeing two different pointers for the same object

bool BaseThread::TriggerGCTrap(Page* page) {
    gcTrap_mutex.lock();
    bool result = gcTrapEnabled && page->Blocked();
    gcTrap_mutex.unlock();
    return result;
}

