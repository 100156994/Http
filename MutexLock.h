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
    void lock()//����MutexLockGuardʹ��
    {
        pthread_mutex_lock(&mutex_);
        holder_=CurrentThread::tid();
    }

    void unlock()//����MutexLockGuardʹ��
    {
        //��conditionһ��ʹ�ûᵼ��״̬��һ�� wait
        holder_=0;
        pthread_mutex_unlock(&mutex_);
    }

    pthread_mutex_t* getThreadMutex()//����Conditionʹ��
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
