
#include"TimerQueue.h"
#include<functional>
#include<string.h>
#include <sys/timerfd.h>

namespace detail
{
    const int kMicroSecondsPerSecond = 100 0000;

    size_t addTime(size_t now, double seconds)
    {
        size_t delta = static_cast<size_t>(seconds*kMicroSecondsPerSecond);
        return now+delta;
    }

    int createTimerfd()
    {
        int timerfd = timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK | TFD_CLOEXEC);
        if(timerfd<0)
        {
            //log error

        }
        return timerfd;
    }

    size_t now()//gettimeofday不是系统调用  精确到微秒足够了  没必要到纳秒用系统调用clock_gettime
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        size_t seconds = tv.tv_sec;
        size_t now = seconds * kMicroSecondsPerSecond + tv.tv_usec;
        return now;
    }

    struct timespec howMuchTimeFromNow(size_t expired)
    {
        size_t mircoSeconds = expired - now():
        if(mircoSeconds<100)
        {
            mircoSeconds = 100;//最近的定时改变为100微秒
        }
        struct timespec  ts;
        ts.tv_sec = static_cast<time_t>(mircoSeconds /kMicroSecondsPerSecond);
        ts.tv_nsec = static_cast<long>(mircoSeconds%kMicroSecondsPerSecond*1000);
        return ts;
    };

    void resetTimerfd(int timerfd,size_t expired)
    {
        struct itimerspec newValue;
        struct itimerspec oldValue;
        memset(newValue,0,sizeof(newValue));
        memset(oldValue,0,sizeof(oldValue));
        newValue.it_value =howMuchTimeFromNow(expired);
        int ret=timerfd_settime(timerfd,0,newValue,oldValue);
        if(ret){
            //error  to-do

        }
    }

    void readTimerfd(int timerfd,size_t now)
    {
        uint64_t n;
        ssize_t ret = read(timerfd,&n,sizeof(uint64_t));
        if(ret!=sizeof(uint64_t))
        {
            //error log
        }
    }
}

using namespace detail;

TimerQueue::TimerQueue(EventLoop* loop)
    :loop_(loop),
     timerfd_(createTimerfd()),
     timerfdChannel_(loop_,timerfd_),
     isCallingExpiredTimers_(false)
{
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue.handleRead,this));
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
    //do not remove channel, since we're in EventLoop::dtor();
    //没懂为什么muduo在析构时有这个注释 感觉注释和代码矛盾   析构时从epoll删除timerfdChannel个人感觉没问题

    //timer 在Queue中是new创建的 在析构是要delete
    for(const Entry& entry:timers_)
    {
        delete entry.second;
    }
}


TimerId TimerQueue::addTimer(TimerCallback cb, size_t when, double interval)
{
    Timer* timer = new Timer(std::move(cb),when,interval);
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop,&this,timer));
    return TimerId(timer,timer->sequence());
}

void TimerQueue::cancelTimer(TimerId timerId)
{
    loop_->runInLoop(std::bind(&TimerQueue::cancelTimerInloop,&this,timerId);
}


void TimerQueue::addTimerInLoop(Timer* timer)
{
    loop_->assertInLoopThread();
    bool earliestChanged = insertTimer(timer);
    if(earliestChanged)
    {
        resetTimerfd(timerfd_,timer->expiredMs());
    }
}


void TimerQueue::cancelTimerInloop(TimerId timerId)
{
    loop_.assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());

    ActiveTimer atimer(timerId.timer_,timerId.sequence_);
    ActiveTimerList::iterator it = activeTimers_.find(atimer);
    if(it!=activeTimers_.end())//在活动timer里
    {
        size_t n = timers_.erase(Entry(it->first->expiredMs(), it->first));
        assert(n == 1);
        delete it->first; // FIXME: no delete please
        activeTimers_.erase(it);
    }else if(isCallingExpiredTimers_)//正在执行过期定时器  此时timer可能在expired中
    {
        cancelTimers_.insert(atimer);
    }else{
        //error
    }
    assert(timers_.size() == activeTimers_.size());
}



bool TimerQueue::insertTimer(Timer* timer)
{
    loop_->assertInLoopThread();
    assert(timers_.size()==activeTimers_.size());
    bool earliestChanged = false;
    size_t expired = timer->expiredMs();

    TimerList::iterator it = timers_.begin();
    if(it == timers_.end()|| expired < it->first)
    {
        earliestChanged = true;
    }

    {
        std::pair<TimerList::iterator,bool> ret =  timers_.insert(Entry(expired,timer));
        assert(ret.second);
    }
    {
        std::pair<ActiveTimerList::iterator,bool> ret = activeTimers_.insert(ActiveTimer(timer,timer->sequence()));
        assert(ret.second);
    }
    assert(timers_.size()==activeTimers_.size());
    return earliestChanged
}

void TimerQueue::handleRead()
{
    loop_.assertInLoopThread();
    size_t now = now();
    readTimerfd(timerfd_,now);

    //获得过期的timer
    std::vector<TimerQueue::Entry> expiredTimers = getExpired(now);
    //执行定时
    isCallingExpiredTimers_ =true ;

    cancelTimers_.clear();//
    for(const Entry& it:expiredTimers)
    {
        it.second->run();
    }

    isCallingExpiredTimers_ = false;
    //重设timerfd

    reset(expiredTimers,now);
}


std::vector<TimerQueue::Entry> getExpired(size_t now)
{
    loop_.assertInLoopThread();
    assert(timers_.size()==activeTimers_.size());

    std::vector<TimerQueue::Entry> expiredTimers;
    Entry sentry(now,reinterpret_cast<Timer*>(UINTPTR_MAX));
    TimerList::iterator end =timers_.lower_bound(sentry);
    assert(end == timers_.end() || now < end->first);

    std::copy(timers_.begin(),end,back_inserter(expiredTimers));
    timers_.erase(timers_.begin(),end);

    for(const Entry& it:expiredTimers)
    {
        ActiveTimer atimer(it.seconde,it.seoncde->sequence);
        size_t n = activeTimers_.erase(atimer);
        assert(n==1);
    }
    assert(timers_.size()==activeTimers_.size());
    return expiredTimers;
}

void reset(const std::vector<TimerQueue::Entry>& expiredTimers,size_t now)
{
    size_t nextExpire;

    for(const Entry& it:expiredTimers)
    {
        ActiveTimer atimer(it.second, it.second->sequence());
        if(it.second->repeat()&&cancelTimers_.find(atimer)==cancelTimers_.end())
        {
            it.second->restart(addTime(now,it.second->interval()));
            insert(it.second);
        }else{
            delete it.second;
        }
    }

    if(!timers_.empty())
    {
        nextExpire = timers_.begin()->second->expiredMs();
    }
    resetTimerfd(timerfd_,nextExpire);
}


