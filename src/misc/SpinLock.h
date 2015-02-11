#pragma once

// simple spin lock implementation based on test_and_set
// Implementation 'taken' from http://anki3d.org/spinlock/
// (no license?, but it's kind of trivial)

class SpinLock {
public:
    void lock() {
        while (lck.test_and_set(std::memory_order_acquire)) {}
    }
    
    void unlock() {
        lck.clear(std::memory_order_release);
    }
    
private:
    std::atomic_flag lck = ATOMIC_FLAG_INIT;
};
