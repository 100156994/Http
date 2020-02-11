#pragma once

#include<functional>
#include<atomic>

#include"noncopyable.h"

class Timer :noncopyable
{
public:
    using TimerCallback = std::function<void()>;
    Timer(TimerCallback cb,size_t when,double interval)
        :callback_(std::move(cb)),
         expired_ms_(when),
         interval_(interval),
         repeat_(interval_>0.0),
         sequence_(++s_numCreated_)
         {
         }
    ~Timer(){};

    void run()const{ callback_();}
    size_t expiredMs()const {return expired_ms_;}
    bool repeat()const{return repeat_;}
    int64_t sequence()const {return sequence_;}
    void restart(size_t now){expired_ms_ = now;}
    double interval()const {return interval_;}

    static int64_t numCreated(){int64_t x = s_numCreated_.load(std::memory_order_relaxed);return x;}
private:
    const TimerCallback callback_;
    size_t expired_ms_;
    const double interval_;
    const bool repeat_;
    const int64_t sequence_;
    static std::atomic<int64_t> s_numCreated_;
};
