
#include"EventLoopThread.h"
#include"EventLoop.h"
#include<assert.h>

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,const string& name)
    : loop_(NULL),
     exiting_(false),
     thread_(std::bind(&EventLoopThread::threadFunc, this), name),
     mutex_(),
     cond_(mutex_),
     callback_(cb)
{
}


EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != NULL) // 主线程要是里进行析构时 如果threadFunc刚刚执行完毕 则有可能判断成功 但之后loop_为NULL
    {
      // 虽然有小几率竞争 但是析构时候程序应该直接退出了 不影响
      loop_->quit();
      thread_.join();
    }
}


EventLoop* EventLoopThread::startLoop()
{
    assert(!thread_.started());
    thread_.start();
    EventLoop* loop = NULL;
    {
        MutexLockGuard lock(mutex_);
        while (loop_ == NULL)
        {
            cond_.wait();
        }
        loop = loop_;
    }
    return loop;
}


void EventLoopThread::threadFunc()
{
    EventLoop loop;

    if(callback_)
    {
        callback_(&loop);
    }
    {
    MutexLockGuard lock(mutex_);
    loop_ = &loop;
    cond_.notify();
    }
    loop.loop();
    MutexLockGuard lock(mutex_);
    loop_ = NULL;
}
