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
    typedef std::pair<size_t,Timer*> Entry;//��һ��Ԫ��Ϊ����ʱ��
    typedef std::set<Entry> TimerList;//����ʱ������
    typedef std::pair<Timer*,int64_t> ActiveTimer;//�ڶ���Ԫ��Ϊtimer sequence ͬһ��ַ������Timer sequence������ͬ
    typedef std::set<ActiveTimer> ActiveTimerList;

    void addTimerInLoop(Timer* timer);
    void cancelTimerInloop(TimerId timerId);
    bool insertTimer(Timer*);//��������ʱ���Ƿ�ı�
    void handleRead();//��ʱ��ʱ����
    std::vector<TimerQueue::Entry> getExpired(size_t now);//�Ƴ����ڶ�ʱ��
    void reset(const std::vector<TimerQueue::Entry>& expiredTimer,size_t now);


    EventLoop* loop_;
    const int timerfd_;
    Channel timerfdChannel_;
    bool isCallingExpiredTimers_;

    TimerList timers_;

    ActiveTimerList activeTimers_;
    ActiveTimerList cancelTimers_;
};
