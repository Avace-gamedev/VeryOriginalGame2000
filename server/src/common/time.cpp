#include <windows.h>
#include <cmath>
#include "common/time.h"

unsigned long long Time::start_time = 0;
unsigned long long Time::freq = 0;

void Time::startNow()
{
    QueryPerformanceCounter((LARGE_INTEGER *)&Time::start_time);
    QueryPerformanceFrequency((LARGE_INTEGER *)&Time::freq);
}

tick_t Time::msToTicks(unsigned long long ms, double period)
{
    return (tick_t)(ms / (period * 1000));
}

unsigned long long Time::now()
{
    unsigned long long now;
    QueryPerformanceCounter((LARGE_INTEGER *)&now);
    return 1000 * (now - Time::start_time) / Time::freq;
}

unsigned long long Time::nowInMilliseconds() { return now(); }

unsigned long long Time::nextDeadline(double period)
{
    unsigned long long now = Time::nowInMilliseconds();
    unsigned long long n_periods = (unsigned long long)ceil((double)now / (period * 1000));
    return (unsigned long long)(n_periods * period * 1000);
}

tick_t Time::nowInTicks(double period)
{
    return msToTicks(now(), period);
}

unsigned long long Time::timeBeforeDeadline(double period)
{
    return nextDeadline(period) - nowInMilliseconds();
}

struct timeval Time::timevalOfLongLong(unsigned long long time)
{
    struct timeval res;
    res.tv_sec = (long)(time / 1000);
    res.tv_usec = (long)((time - res.tv_sec) * 1000);
    return res;
}