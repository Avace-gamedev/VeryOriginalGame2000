#pragma once

#include <stdint.h>

typedef uint64_t tick_t;

class Time
{
    static unsigned long long start_time;
    static unsigned long long freq;

public:
    static inline tick_t msToClientTicks(unsigned long long ms);

    static void startNow();
    static unsigned long long now();
    static unsigned long long nowInMilliseconds();
    static tick_t Time::nowInTicks(double period);
    static unsigned long long nextDeadline(double period);
    static unsigned long long timeBeforeDeadline(double period);
    static struct timeval timevalOfLongLong(unsigned long long time);
};