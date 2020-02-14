#pragma once

#include"MutexLock.h"
#include<pthread.h>
#include<time.h>
#include<errno.h>

class Condition : noncopyable{
public:
    explicit Condition(MutexLock& mutex)
    :mutex_(mutex)
    {
        pthread_cond_init(&cond_,NULL);
    }

    ~Condition()
    {
        pthread_cond_destory(&cond_);
    }

    void wait()
    {
        pthread_cond_wait(&cond_,mutex_.getThreadMutex());
    }

    void notify()
    {
        pthread_cond_signal(&cond);
    }

    void notifyAll()
    {
        pthread_cond_broadcast(&cond);
    }

    bool waitForSeconds(double seconds);

private:
    MutexLock& mutex_;
    pthread_cond_t cond_;
};

bool Condition::waitForSeconds(double seconds)
{
  struct timespec abstime;
  // FIXME: 系统时间可能被改?
  clock_gettime(CLOCK_REALTIME, &abstime);

  const int64_t kNanoSecondsPerSecond = 1000000000;
  int64_t nanoseconds = static_cast<int64_t>(seconds * kNanoSecondsPerSecond);

  abstime.tv_sec += static_cast<time_t>((abstime.tv_nsec + nanoseconds) / kNanoSecondsPerSecond);
  abstime.tv_nsec = static_cast<long>((abstime.tv_nsec + nanoseconds) % kNanoSecondsPerSecond);

  return ETIMEDOUT == pthread_cond_timedwait(&cond_, mutex_.getPthreadMutex(), &abstime);
}
