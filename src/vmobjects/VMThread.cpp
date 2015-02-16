#include "VMThread.h"
#include "VMString.h"
#include "VMClass.h"

#include <vm/SafePoint.h>
#include <vm/Universe.h>

#include <sstream>


const size_t VMThread::VMThreadNumberOfGcPtrFields = 1;
mutex VMThread::threads_map_mutex;
map<thread::id, GCThread*> VMThread::threads;


VMThread::VMThread() :
                VMObject(VMThreadNumberOfGcPtrFields),
                    thread(nullptr),
                    name(reinterpret_cast<GCString*>(nilObject)) {}

VMString* VMThread::GetName() {
    return load_ptr(name);
}

void VMThread::SetName(VMString* val) {
    store_ptr(name, val);
}

void VMThread::SetThread(std::thread* t) {
    thread = t;
}

void VMThread::Join() {
# warning there is a race condition on the thread field, \
  because it is set after construction, \
  thank you C++11 API which doesn't allow me to create threads without executing them.
    try {
        // this join() needs always to be in tail position with respect to all
        // operations, before going back to the interpreter loop.
        // Specifically, we can't use any object pointers that might have moved.
        
        
#if GC_TYPE==PAUSELESS
        GetUniverse()->GetInterpreter()->EnableBlocked();
#else
        SafePoint::AnnounceBlockingMutator();
#endif

        thread->join();
        
#if GC_TYPE==PAUSELESS
        GetUniverse()->GetInterpreter()->DisableBlocked();
#else
        SafePoint::ReturnFromBlockingMutator();
#endif
    } catch(const std::system_error& e) {
        Universe::ErrorPrint("Error when joining thread: error code " +
                             to_string(e.code().value()) + " meaning " +
                             e.what() + "\n");
    }
}

StdString VMThread::AsDebugString() {
    auto id = thread->get_id();
    stringstream id_ss;
    id_ss << id;
    
    VMString* n = load_ptr(name);
    if (n == reinterpret_cast<VMString*>(load_ptr(nilObject))) {
        return "Thread(id: " + id_ss.str() + ")";
    } else {
        return "Thread(" + n->GetStdString() + ", " + id_ss.str() + ")";
    }
}

VMThread* VMThread::Clone(Page* page) {
// TODO: Clone() should be renamed to Move or Reallocate or something,
// it should indicate that the old copy is going to be invalidated.
    VMThread* clone = new (page, 0 ALLOC_MATURE) VMThread();
    clone->clazz  = clazz;
    clone->thread = thread;
    clone->name   = name;
    return clone;
}

void VMThread::MarkObjectAsInvalid() {
    VMObject::MarkObjectAsInvalid();
    thread = nullptr;
}

void VMThread::Yield() {
    this_thread::yield();
}

VMThread* VMThread::Current() {
    thread::id id = this_thread::get_id();
    
    lock_guard<mutex> lock(threads_map_mutex);
    auto thread_i = threads.find(id);
    if (thread_i != threads.end()) {
        return load_ptr(thread_i->second);
    } else {
        Universe::ErrorExit("Did not find object for current thread. "
                            "This is a bug, i.e., should not happen.");
        return reinterpret_cast<VMThread*>(load_ptr(nilObject));
    }
}

void VMThread::Initialize() {
    lock_guard<mutex> lock(threads_map_mutex);
    
    // this is initialization time, should be done before any GC stuff starts
    // so, don't need a write barrier
    threads[this_thread::get_id()] = reinterpret_cast<GCThread*>(nilObject);
    SafePoint::RegisterMutator(); // registers the main thread with the safepoint
}

void VMThread::RegisterThread(thread::id threadId, VMThread* thread) {
    lock_guard<mutex> lock(threads_map_mutex);
    
    auto thread_i = threads.find(threadId);
    assert(thread_i == threads.end()); // should not be in the map
    
    threads[threadId] = to_gc_ptr(thread);

    // SafePoint::RegisterMutator() is already done as part of thread creation.
    // This is necessary, to make sure that we do not get a GC, before it is
    // safe to do so. Otherwise the thread, block, and argument pointer are
    // invalidated by a moving GC
}

void VMThread::UnregisterThread(thread::id threadId) {
    lock_guard<mutex> lock(threads_map_mutex);
    
    size_t numRemoved = threads.erase(threadId);
    assert(numRemoved == 1);
    
    SafePoint::UnregisterMutator();
}

void VMThread::WalkGlobals(walk_heap_fn walk, Page* page) {
    lock_guard<mutex> lock(threads_map_mutex);
    
    for (auto& pair : threads) {
        do_walk(pair.second);
    }
}
