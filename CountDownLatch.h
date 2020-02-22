#pragma once

#include"MutexLock.h"
#include"Condition.h"
#include"noncopyable.h"


//倒计时 用于创建线程前先初始化一些线程独有变量和线程ID
class CountDownLatch{

public:
    explicit CountDownLatch(int count)
        :mutex_(),
         cond_(mutex_),
         count_(count)
    {
    }

    void wait()
    {
        MutexLockGuard lock(mutex_);
        while(count_>0){
            cond_.wait();
        }
    }

    void countDown()
    {
        MutexLockGuard lock(mutex_);
        --count_;
        if(count_==0)
        {
            cond_.notifyAll();
        }
    }

    int getCount()const
    {
        MutexLockGuard lock(mutex_);
        return count_;
    }

private:
    mutable MutexLock mutex_;//get 为常量成员 加锁改变mutex
    Condition cond_;
    int count_;

};
