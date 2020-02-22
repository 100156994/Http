#pragma once

#include"Timer.h"
#include"noncopyable.h"

class Timer;

//Timer*+sequence_ 唯一标识一个timer 即使timer过期
class TimerId
{
public:
    TimerId()
        :timer_(nullptr),
         sequence_(0)
         {
         }

    TimerId(Timer* timer,int64_t sequence)
        :timer_(timer),
         sequence_(sequence)
         {
         }
    friend class TimerQueue;

private:
    Timer* timer_;
    int64_t sequence_;
};
