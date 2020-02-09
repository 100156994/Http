#include"EventLoop.h"


__thread EventLoop* t_loopInThisThread = nullptr;
const int kPollTimeMs = 10000;

EventLoop::EventLoop()
    :looping_(false),
    quit_(false),
    threadId_(CurrentThread::tid()),
    eventHanding_(false),
    callPendingFunctors_(false),
    poller_(new Epoller(this)),
    currentActiveChannel_(nullptr)
{
    if(t_loopInThisThread)//��ǰ�߳��Ѿ���һ��loop��
    {
        //log
    }else
    {
        t_loopInThisThread = this;
    }

}

EventLoop::~EventLoop()
{
    assert(!looping_);
    t_loopInThisThread = nullptr;
}



void EventLoop::loop()
{
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;
    while(!quit_){
        activeChannels_.clear();
        poller_.poll(kPollTimeMs,&activeChannels_);

        eventHanding_ = true;
        for(Channel* channel:activeChannels_)
        {
            channel.handleEvent();
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

void EventLoop::wakeup()//�첽����
{
    //to-do
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
