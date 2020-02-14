#pragma once

#include"MutexLock.h"
#include"Condition.h"
#include"Thread.h"

class EventLoop;


class EventLoopThread :noncopyable
{

public:
    typedef std::function<void(EventLoop*)> ThreadInitCallback;

    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                  const string& name = string());
    ~EventLoopThread();
    EventLoop* startLoop();


private:
    void ThreadFunc();

    EventLoop* loop_;//锁保护
    bool exiting_;
    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
    ThreadInitCallback callback_;

};
