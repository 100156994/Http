
#pragma once
#include"Epoll.h"
#include"CurrentThread.h"
#include<functional>

class EventLoop
{
public:
    typedef std::function<void()> Functor;
    EventLoop();
    ~EventLoop();

    void loop();
    void runInLoop(Functor& cb);
    void queueInLoop(Functor& cb);

    void wakeup();
    size_t queueSize()const ;


    bool eventHanding()const {return eventHanding_;}

    static EventLoop* getEventLoopOfCurrentThead();
    bool isLoopInThread()const { return threadId_ == CurrentThread::tid();}
    void assertInLoopThread()
    {
        if(!isLoopInThread())
        {
            abortNotInLoopThread();
        }
    }

private:
    void abortNotInLoopThread();
    void doPendingFunctors();

    typedef std::vector<Channel*> ChannelList;

    bool looping_;
    bool quit_;
    const pid_t threadId_;
    bool eventHanding_;
    bool callPendingFunctors_;
    std::unique<Epoller> poller_;

    ChannelList activeChannels_;
    Channel* currentActiveChannel_;

    mutable MutexLock mutex_;
    std::vector<Functor> pendingFunctors_;
}
