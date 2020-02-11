#pragma once


#include<stdint.h>
#include<set>
#include<vector>
#include"Timer.h"
#include"TimerId.h"
#include"EventLoop.h"
#include"noncopyable.h"

class EventLoop;
class Timer;
class TimerId;

class TimerQueue : noncopyable
{
public:
    using TimerCallback = std::function<void()>;
    TimerQueue(EventLoop* loop)

    ~TimerQueue();

    TimerId addTimer(TimerCallback cb,size_t when,double interval );

    void cancelTimer(TimerId timerId);


private:
    typedef std::pair<size_t,Timer*> Entry;//第一个元素为过期时间
    typedef std::set<Entry> TimerList;//按照时间排序
    typedef std::pair<Timer*,int64_t> ActiveTimer;//第二个元素为timer sequence 同一地址的两个Timer sequence不会相同
    typedef std::set<ActiveTimer> ActiveTimerList;

    void addTimerInLoop(Timer* timer);
    void cancelTimerInloop(TimerId timerId);
    bool insertTimer(Timer*);//返回最早时间是否改变
    void handleRead();//定时到时调用
    std::vector<TimerQueue::Entry> getExpired(size_t now);//移除过期定时器
    void reset(const std::vector<TimerQueue::Entry>& expiredTimer,size_t now);


    EventLoop* loop_;
    const int timerfd_;
    Channel timerfdChannel_;
    bool isCallingExpiredTimers_;

    TimerList timers_;

    ActiveTimerList activeTimers_;
    ActiveTimerList cancelTimers_;
};
