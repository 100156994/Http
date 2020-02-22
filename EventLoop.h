
#pragma once
#include"Epoll.h"
#include"CurrentThread.h"
#include"Channel.h"
#include"TimerQueue.h"
#include<functional>
#include"MutexLock.h"
#include<memory>
#include<string>
#include <unistd.h>
#include"Logging.h"
class Epoller;
class Channel;
class TimerQueue;

using  std::string;


//一个线程只能有一个loop  跨线程调用使用锁保护  处理完所有epoll所有活动时间后  才会执行跨线程任务  不会递归添加，而是异步唤醒下一轮执行
//有一个定时器队列 和异步唤醒channel fd(由loop本身负责关闭)
class EventLoop
{
public:
    typedef std::function<void()> Functor;
    typedef std::function<void()> TimerCallback;
    EventLoop();
    ~EventLoop();

    //只能在创建线程调用
    void loop();
    void runInLoop(Functor cb);
    void queueInLoop(Functor cb);
    size_t queueSize()const ;

    void quit();//跨线程调用有一定问题

    //定时器任务
    TimerId runAt(size_t time, TimerCallback cb);
    TimerId runAfter(double delay, TimerCallback cb);
    TimerId runEvery(double interval, TimerCallback cb);
    void cancel(TimerId timerId);

    //internal  其他线程异步唤醒 和chanel-remove
    void wakeup();
    void updateChannel(Channel*);
    void removeChannel(Channel*);
    bool hasChannel(Channel*);




    bool eventHanding()const {return eventHanding_;}


    bool isLoopInThread()const { return threadId_ == CurrentThread::tid();}
    void assertInLoopThread()
    {
        if(!isLoopInThread())
        {
            abortNotInLoopThread();
        }
    }
    static EventLoop* getEventLoopOfCurrentThread();
private:
    void abortNotInLoopThread();
    void doPendingFunctors();
    void handleRead();



    typedef std::vector<Channel*> ChannelList;

    bool looping_;
    bool quit_;
    const pid_t threadId_;
    bool eventHanding_;
    bool callPendingFunctors_;
    std::unique_ptr<Epoller> poller_;
    std::unique_ptr<TimerQueue> timerQueue_;

    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;


    ChannelList activeChannels_;
    Channel* currentActiveChannel_;

    mutable MutexLock mutex_;
    std::vector<Functor> pendingFunctors_;
};
