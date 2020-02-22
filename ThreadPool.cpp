

#include"ThreadPool.h"
#include<assert.h>
#include<exception>
#include<stdio.h>

ThreadPool::ThreadPool(const string name)
    :mutex_(),
     notFull_(mutex_),
     notEmpty_(mutex_),
     name_(name),
     maxQueueSize_(0),
     running_(false)
     {
     }

ThreadPool::~ThreadPool()
{
    if(running_)
    {
        stop();
    }
}

void ThreadPool::start(int numThread)
{
    assert(threads_.empty());
    assert(running_);
    running_ = true;
    threads_.reserve(numThread);
    for( int i = 0; i < numThread ;++i)
    {
        char id[32];
        snprintf(id,sizeof id,"%d",i+1);

        threads_.emplace_back(new Thread(std::bind(&ThreadPool::runInThread,this),name_+id));
        threads_[i]->start();
    }
}

void ThreadPool::run(const Task f)
{
    if(threads_.empty())
    {
        f();
    }else
    {
        MutexLockGuard lock(mutex_);
        while(isFull())
        {
            notFull_.wait();
        }
        assert(!isFull());
        queue_.push_back(std::move(f));
        notEmpty_.notify();
    }
}

void ThreadPool::stop()
{
    MutexLockGuard lock(mutex_);
    running_ = false;
    notEmpty_.notifyAll();
    //等待所有线程结束
    for(auto& thr:threads_)
    {
        thr->join();
    }
}

//添加任务时调用
bool ThreadPool::isFull()const
{
    return  maxQueueSize_>0 && maxQueueSize_ >= queue_.size();
}


void ThreadPool::runInThread()
{
    try
    {
        assert(running_);
        if(threadInitCallback_)
        {
            threadInitCallback_();
        }
        while(running_)
        {
            Task task(take());
            if(task)
            {
                task();
            }
        }
    }catch(const std::exception& ex)
    {
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        abort();
    }catch(...)
    {
        fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
        throw; // rethrow
    }
}

ThreadPool::Task ThreadPool::take()
{
    MutexLockGuard lock(mutex_);
    while(queue_.empty()&&running_)
    {
        notEmpty_.wait();
    }
    Task task;
    if(queue_.empty())
    {
        task = queue_.front();
        queue_.pop_front();
        notFull_.notify();
    }
    return task;
}
