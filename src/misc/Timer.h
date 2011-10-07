#pragma once
#ifndef TIMER_H
#define TIMER_H

#include <time.h>

class Timer {
    private:
        timespec total;
        timespec last_start;
    public:
        static Timer* GCTimer;
        inline void Resume() {
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &last_start);
        }
        inline void Halt() {
            timespec end;
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
            total.tv_sec += end.tv_sec - last_start.tv_sec;
            total.tv_nsec += end.tv_nsec - last_start.tv_nsec;
            if (total.tv_nsec < 0) {
                total.tv_nsec += 1000000000;
                total.tv_sec--;
            }
            else if (total.tv_nsec >= 1000000000) {
                total.tv_nsec -= 1000000000;
                total.tv_sec++;
            }
        }

        double GetTotalTime() {
            return total.tv_sec*1000 + total.tv_nsec / 1000000.0;
        }

};

#endif
#pragma once
#ifndef TIMER_H
#define TIMER_H

#include <time.h>

class Timer {
    private:
        timespec total;
        timespec last_start;
    public:
        static Timer* GCTimer;
        inline void Resume() {
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &last_start);
        }
        inline void Halt() {
            timespec end;
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
            total.tv_sec += end.tv_sec - last_start.tv_sec;
            total.tv_nsec += end.tv_nsec - last_start.tv_nsec;
            if (total.tv_nsec < 0) {
                total.tv_nsec += 1000000000;
                total.tv_sec--;
            }
            else if (total.tv_nsec >= 1000000000) {
                total.tv_nsec -= 1000000000;
                total.tv_sec++;
            }
        }

        double GetTotalTime() {
            return total.tv_sec*1000 + total.tv_nsec / 1000000.0;
        }

};

#endif
