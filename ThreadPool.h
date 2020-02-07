#pragma once

#include<string>
#include<function>
#include<vector>
#include<deque>
#include"Thread.h"
#include"noncopyable.h"
#include"MutexLock.h"
#include"Condition.h"

using std::string;

class ThreadPool:noncopyable
{
    typedef std::function<void()> Task;
    explicit ThreadPool(const string name = string());
    ~ThreadPool();

    //start之前调用
    void setInitCallBack(const Task& f){ threadInitCallback_ = f;}
    void setMaxQueueSize(int maxSize){ maxQueueSize_ = maxSize;}

    void start(int numThread);
    void run(const Task f);
    void stop();

    const string& name()const {return name_;}
    size_t queueSize()const
    {
        MutexLockGuard lock(mutex_);
        return queue_.size();
    }

private:
    bool isFull()const;
    void runInThread();
    void take();
    MutexLock mutex_;
    Condition notFull_;
    Condition notEmpty_;
    string name_;
    Task threadInitCallback_;
    std::vector<unique_ptr<Thread>> threads_;
    std::deque<Task> queue_;
    size_t maxQueueSize_;
    bool running_;
};
