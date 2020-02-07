#pragma once

#include<pthread.h>
#include<assert>
#include"noncopyable.h"


class MutexLock : noncopyable{

public:
    MutexLock()
        :holder_(0)
    {
        pthread_mutex_init(&mutex_,NULL)
    }

    ~MutexLock()
    {
        assert(holder_==0);
        pthread_mutex_destory(&mutex_);
    }
    bool isLockedByThisThread()
    {
        return holder_==CurrentThread::tid();
    }
    void assertLocked()
    {
        assert(isLockedByThisThread());
    }
    void lock()//仅供MutexLockGuard使用
    {
        pthread_mutex_lock(&mutex_);
        holder_=CurrentThread::tid();
    }

    void unlock()//仅供MutexLockGuard使用
    {
        //和condition一起使用会导致状态不一致 wait
        holder_=0;
        pthread_mutex_unlock(&mutex_);
    }

    pthread_mutex_t* getThreadMutex()//仅供Condition使用
    {
        return &mutex_;
    }

private:
    pthread_mutex_t mutex_;
    pid_t holder_;
};

class MutexLockGuard : noncopyable{
public:
    explicit MutexLockGuard(MutexLock & mutex)
        :mutex_(mutex);
    {
        mutex_.lock();
    }

    ~MutexLockGuard()
    {
        mutex_.unlock();
    }


private:
    MutexLock& mutex_;
};


#define MutexLockGuard(x) static_assert(false,"missing mutex guard var name")
