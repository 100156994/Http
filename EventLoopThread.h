#pragma once

#include"MutexLock.h"
#include"Condition.h"
#include"Thread.h"

class EventLoop;

//loop线程 执行简单的线程初始化 和cb 准备后返回loop*
class EventLoopThread :noncopyable
{

public:
    typedef std::function<void(EventLoop*)> ThreadInitCallback;

    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                  const string& name = string());
    ~EventLoopThread();
    EventLoop* startLoop();


private:
    void threadFunc();

    EventLoop* loop_;//锁保护
    bool exiting_;
    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
    ThreadInitCallback callback_;

};
