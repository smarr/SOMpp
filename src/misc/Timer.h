#pragma once

#include <time.h>
#include <stdint.h>
#include <sys/time.h>

static int64_t get_microseconds() {
#if defined(CLOCK_PROCESS_CPUTIME_ID)
    // this is for Linux
    timespec now;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);

    return (now.tv_sec * 1000 * 1000) +// seconds
    (now.tv_nsec / 1000);// nanoseconds
#else
    // this is for OSX, might work on other platforms
    struct timeval now;
    gettimeofday(&now, nullptr);

    return (now.tv_sec * 1000 * 1000) +  // seconds
            now.tv_usec;                 // microseconds
#endif
}

class Timer {
private:
    int64_t total;
    int64_t last_start;
public:
    static Timer* GCTimer;
    inline void Resume() {
        last_start = get_microseconds();
    }
    inline void Halt() {
        int64_t end = get_microseconds();

        total = end - last_start;
    }

    double GetTotalTime() {
        return total / 1000.0;
    }

};
