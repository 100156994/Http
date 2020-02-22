
#include"EventLoopThreadPool.h"
#include"EventLoopThread.h"
#include"EventLoop.h"
#include<stdio.h>

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const string& nameArg)
    :baseLoop_(baseLoop),
     name_(nameArg),
     started_(false),
     numThreads_(0),
     next_(0)
    {
    }

EventLoopThreadPool::~EventLoopThreadPool()
{
  //由 vector unique_ptr 自动管理
}


void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
    assert(!started_);
    baseLoop_->assertInLoopThread();
    started_ = true;

    for (int i = 0; i < numThreads_; ++i)
    {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        EventLoopThread* t = new EventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop());
    }
    if (numThreads_ == 0 && cb)
    {
        cb(baseLoop_);
    }

}


// 不是线程安全的 只能base线程访问   不然需要对next_加锁
EventLoop* EventLoopThreadPool::getNextLoop()
{
    baseLoop_->assertInLoopThread();
    assert(started_);
    EventLoop* loop = baseLoop_;

    if(!loops_.empty())
    {
        loop = loops_[next_];
        ++next_;
        if(static_cast<size_t>(next_) == loops_.size())
        {
            next_=0;
        }
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
    baseLoop_->assertInLoopThread();
    assert(started_);
    if(!loops_.empty())
    {
        return loops_;
    }else
    {
        return std::vector<EventLoop*>(1,baseLoop_);
    }
}
