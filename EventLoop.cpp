#include"EventLoop.h"
#include<sys/eventfd.h>
#include<unistd.h>
#include<algorithm>
#include <inttypes.h>  
#include<signal.h>

using namespace detail;

namespace detail
{
    //const int kMicroSecondsPerSecond = 1000000;

  
    int createEventfd()
    {
        int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (evtfd < 0)
        {
            //log
        }
        return evtfd;
    }

    void writeEventfd(int wakeupFd)
    {
        uint64_t one = 1;
        ssize_t n = write(wakeupFd, &one, sizeof(one));
        if (n != sizeof(one))
        {
            //LOG_ERROR
        }
    }

    void readEventfd(int wakeupFd)
    {
        uint64_t one = 1;
        ssize_t n = read(wakeupFd, &one, sizeof one);
        if (n != sizeof one)
        {
            //LOG_ERROR
        }
    }

}

using namespace detail;

class IgonreSigPipe
{
public:
    IgonreSigPipe()
    {
        ::signal(SIGPIPE,SIG_IGN);
    }
};

IgonreSigPipe initObj;


__thread EventLoop* t_loopInThisThread = nullptr;
const int kPollTimeMs = 10000;

EventLoop::EventLoop()
    :looping_(false),
    quit_(false),
    threadId_(CurrentThread::tid()),
    eventHanding_(false),
    callPendingFunctors_(false),
    poller_(new Epoller(this)),
    timerQueue_(new TimerQueue(this)),
    wakeupFd_(createEventfd()),
    wakeupChannel_(new Channel(this,wakeupFd_)),
    currentActiveChannel_(nullptr)
{
    if(t_loopInThisThread)//当前线程已经有一个loop了
    {
        //log
	//LOG<<"this thread already has a loop";
    }else
    {
        t_loopInThisThread = this;
    }
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    assert(!looping_);
    t_loopInThisThread = nullptr;
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
}



void EventLoop::loop()
{
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;
    while(!quit_){
        activeChannels_.clear();
        size_t receiveTime = poller_->poll(kPollTimeMs,&activeChannels_);

        eventHanding_ = true;
        for(Channel* channel:activeChannels_)
        {
            currentActiveChannel_ = channel;
            channel->handleEvent(receiveTime);
        }
        eventHanding_ = false;
        doPendingFunctors();
    }

    looping_ = false;
}

void EventLoop::runInLoop(Functor cb)
{
    if(isLoopInThread())
    {
        cb();
    }else{
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.push_back(cb);
    }
    if(!isLoopInThread()||callPendingFunctors_)
    {
        wakeup();
    }
}

void EventLoop::quit()
{
  quit_ = true;
  // 在loop之前 quit 那么在loop线程可能 eventloop正在析构 然后当前线程还在调用这个对象的函数 理论上应该在quit和主线程加锁
  //

  if (!isLoopInThread())
  {
    wakeup();
  }
}


TimerId EventLoop::runAt(size_t time, TimerCallback cb)
{
    return timerQueue_->addTimer(cb,time,0.0);
}
TimerId EventLoop::runAfter(double delay, TimerCallback cb)
{
    return timerQueue_->addTimer(cb,addTime(now(),delay),0.0);
}
TimerId EventLoop::runEvery(double interval, TimerCallback cb)
{
    return timerQueue_->addTimer(cb,addTime(now(),interval),interval);
}
void EventLoop::cancel(TimerId timerId)
{
    timerQueue_->cancelTimer(timerId);
}


void EventLoop::wakeup()//异步唤醒
{
    //to-do
    writeEventfd(wakeupFd_);
}

void EventLoop::updateChannel(Channel* channel)
{
    assertInLoopThread();
    assert(channel->ownerLoop()==this);
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
    assertInLoopThread();
    assert(channel->ownerLoop()==this);
    if (eventHanding_)
    {
        assert(currentActiveChannel_ == channel ||
        std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());//这里断言
    }
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
    assertInLoopThread();
    assert(channel->ownerLoop()==this);
    return poller_->hasChannel(channel);
}


EventLoop* EventLoop::getEventLoopOfCurrentThread() //静态成员
{
    return t_loopInThisThread;
}


void EventLoop::abortNotInLoopThread()
{
    //log
    //LOG<<"abort not in loop thread";
    abort();
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callPendingFunctors_ = true;

    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    //printf("do func\n");
    for(const Functor& functor:functors)
    {
        functor();
    }
    callPendingFunctors_ =false;
    //printf("finish func\n");
}

void EventLoop::handleRead()
{
    readEventfd(wakeupFd_);
}
