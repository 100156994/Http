#include"EventLoop.h"
#include<sys/eventfd.h>

#include<unistd.h>

#include<algorithm>
#include<signal.h>
namespace detail
{
    int createEventfd()
    {
        int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (evtfd < 0)
        {
            //log
        }
        return evtfd;
    }

    void writeEventfd()
    {
        uint64_t one = 1;
        ssize_t n = sockets::write(wakeupFd_, &one, sizeof(one));
        if (n != sizeof(one))
        {
            //LOG_ERROR
        }
    }

    void readEventfd()
    {
        uint64_t one = 1;
        ssize_t n = sockets::read(wakeupFd_, &one, sizeof one);
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
        size_t receiveTime = poller_.poll(kPollTimeMs,&activeChannels_);

        eventHanding_ = true;
        for(Channel* channel:activeChannels_)
        {
            currentActiveChannel_ = channel;
            channel.handleEvent(receiveTime);
        }
        eventHanding_ = false;
        doPendingFunctors();
    }

    looping_ = false;
}

void EventLoop::runInLoop(Functor& cb)
{
    if(isLoopInThread())
    {
        cb();
    }else{
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(Functor& cb)
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

  if (!isInLoopThread())
  {
    wakeup();
  }
}


TimerId EventLoop::runAt(size_t time, TimerCallback cb)
{
    timerQueue_->addTimer(cb,time,0.0);
}
TimerId EventLoop::runAfter(double delay, TimerCallback cb)
{
    timerQueue_->addTimer(cb,addTime(now()+delay),0.0);
}
TimerId EventLoop::runEvery(double interval, TimerCallback cb)
{
    timerQueue_->addTimer(cb,addTime(now()+interval),interval);
}
void EventLoop::cancel(TimerId timerId)
{
    timerQueue_->cancelTimer(TimerId);
}


void EventLoop::wakeup()//异步唤醒
{
    //to-do
    writeEventfd();
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
    if (eventHandling_)
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
    return poller_>hasChannel(channel);
}


EventLoop* EventLoop::getEventLoopOfCurrentThead() //静态成员
{
    return t_loopInThisThread;
}


void EventLoop::abortNotInLoopThread()
{
    //log
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callPendingFunctors_ = true;

    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for(const Functor& functor:functors)
    {
        functor();
    }
    callPendingFunctors_ =false;
}

void EventLoop::handleRead()
{
    readEventfd();
}
