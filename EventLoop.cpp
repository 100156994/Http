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
    if(t_loopInThisThread)//��ǰ�߳��Ѿ���һ��loop��
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
  // ��loop֮ǰ quit ��ô��loop�߳̿��� eventloop�������� Ȼ��ǰ�̻߳��ڵ����������ĺ��� ������Ӧ����quit�����̼߳���
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


void EventLoop::wakeup()//�첽����
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
        std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());//�������
    }
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
    assertInLoopThread();
    assert(channel->ownerLoop()==this);
    return poller_>hasChannel(channel);
}


EventLoop* EventLoop::getEventLoopOfCurrentThead() //��̬��Ա
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
