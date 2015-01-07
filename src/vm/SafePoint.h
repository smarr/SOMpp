#pragma once

#include <atomic>
#include <mutex>
#include <iostream>

#include <assert.h>


class SafePoint {
public:
    // register a thread as being a mutator in the system
    static void RegisterMutator() {
        std::lock_guard<std::mutex> lock(mutex);
        
        numTotalMutators++;
        numActiveMutators++;
        
        assert(numActiveMutators > 0);
        assert(numActiveMutators <= numTotalMutators);
    }
    
    static void UnregisterMutator() {
        std::lock_guard<std::mutex> lock(mutex);
        
        numTotalMutators--;
        assert(numTotalMutators >= 0);
        
        doSafePointWithoutBlocking(true);
    }
    
    static void ReachSafePoint(std::function<void()> action) {
        std::unique_lock<std::mutex> lock(mutex);
        
        if (action_fn == nullptr) {
            action_fn = action;
        } else {
            // currently, only one action is supported
            // sigh, equality doesn't work...
            // assert(action_fn == action);
        }
        
        doSafePoint(lock);
    }
    
    static void AnnounceBlockingMutator() {
        std::lock_guard<std::mutex> lock(mutex);
        // perform the normal safepoint logic,
        // which reduces and increase numActivateMutators,
        doSafePointWithoutBlocking(true);
    }
    
    static void ReturnFromBlockingMutator() {
        std::lock_guard<std::mutex> lock(mutex);
        numActiveMutators++;
    }
    
private:
    
    static void doSafePoint(std::unique_lock<std::mutex>& lock) {
        if (!doSafePointWithoutBlocking(false)) {
            awaitSafePoint(lock);
        } else {
            assert(1 == numActiveMutators);
        }
    }
    
    static bool doSafePointWithoutBlocking(bool blockExternally) {
        numActiveMutators--;
        
        bool isLast = numActiveMutators == 0;
        if (isLast) {
            completeSafePoint(blockExternally);
        }
        return isLast;
    }
    
    static void completeSafePoint(bool blockExternally) {
        if (action_fn) {
            action_fn();
        }
        
        // prepare next safepoint
        safepoint_i++;
        action_fn = nullptr;
        if (!blockExternally) {
            numActiveMutators++;
        }
        
        // wake all other mutators
        condvar.notify_all();
    }
    
    static void awaitSafePoint(std::unique_lock<std::mutex>& lock) {
        int64_t current_sp = safepoint_i;
        
        while (current_sp == safepoint_i) {
            condvar.wait(lock);
        }
        
        numActiveMutators++;
    }
    
    static int32_t numTotalMutators;
    static int32_t numActiveMutators;
    
    // iteration of the safepoint, needed to make sure we wake up correctly
    static int64_t safepoint_i;
    
    static std::function<void()> action_fn;
    
    static std::mutex mutex;
    static std::condition_variable condvar;

};
