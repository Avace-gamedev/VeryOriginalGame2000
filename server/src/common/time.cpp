#include <windows.h>
#include <cmath>
#include "common/time.h"

tick_t Time::start_time = 0;
tick_t Time::freq = 0;

void Time::startNow()
{
    QueryPerformanceCounter((LARGE_INTEGER *)&Time::start_time);
    QueryPerformanceFrequency((LARGE_INTEGER *)&Time::freq);
}

tick_t Time::now()
{
    tick_t now;
    QueryPerformanceCounter((LARGE_INTEGER *)&now);
    return 1000 * (now - Time::start_time) / Time::freq;
}

tick_t Time::nowInMilliseconds() { return now(); }

tick_t Time::nextDeadline(double period)
{
    tick_t now = Time::nowInMilliseconds();
    tick_t n_periods = (tick_t)ceil((double)now / (period * 1000));
    return n_periods * period * 1000;
}

tick_t Time::nowInTicks(double period)
{
    return floor(Time::nowInMilliseconds() / (period * 1000));
}

tick_t Time::timeBeforeDeadline(double period)
{
    return nextDeadline(period) - nowInMilliseconds();
}

struct timeval Time::timevalOfLongLong(tick_t time)
{
    struct timeval res;
    res.tv_sec = time / 1000;
    res.tv_usec = (time - res.tv_sec) * 1000;
    return res;
}