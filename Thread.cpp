#pragma once

#include"Thread.h"
#include<assert.h>
#include <errno.h>
#include<utility>
#include <unistd.h>
#include <sys/prctl.h>
#include<sys/syscall.h>
#include"CurrentThread.h"


namespace CurrentThread
{
__thread int t_cachedTid = 0;
__thread char t_tidString[32];
__thread int t_tidStringLength = 6;
__thread const char* t_threadName = "default";

static_assert(std::is_same<int,pid_t>::value,"pid should be int");
}

pid_t gettid()
{
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

void CurrentThread::cacheTid()
{
    if(t_cachedTid==0)
    {
        t_cachedTid = gettid();
        t_tidStringLength = snprintf(t_tidString,sizeof t_tidString,"%d",t_cachedTid);
    }
}

struct ThreadData
{
    typedef Thread::ThreadFunc ThreadFunc;
    ThreadFunc func_;
    string name_;
    pid_t* tid_ptr_;
    CountDownLatch* latch_ptr_;

    ThreadData(ThreadFunc& func,const string name,pid_t* tid_ptr,CountDownLatch* latch_ptr)
        :func_(func),
         name_(name),
         tid_ptr_(tid_ptr),
         latch_ptr_(latch_ptr){}

    void runInThread()
    {
        *tid_ptr_ = CurrentThread::tid();
        tid_ptr_=nullptr;
        latch_ptr_->countDown();//��֤start����֮ǰ�Լ���ȡ��tid
        latch_ptr_=nullptr;

        CurrentThread::t_threadName = name_.empty()?"Thread":name_.c_str();
        ::prctl(PR_SET_NAME,CurrentThread::t_threadName);
        //func����֮ǰ ���ú��߳�id��name
        func_();
        CurrentThread::t_threadName = "finished";
    }
};


Thread::Thread(const ThreadFunc& func, const string& name)
    :started_(false),
     joined_(false),
     pthreadId_(0),
     tid_(0),
     func_(func),
     name_(name),
     latch_(1)
     {
        setDefaultName();
     }

Thread::~Thread()
{
    if(started_&&!joined_)
    {
        pthread_detach(pthreadId_);
    }
}

void Thread::setDefaultName()
{
    //��Thread���洴���߳���������Ը�ÿ���߳�Ĭ�����ּ������
    if (name_.empty()) {
        char buf[32];
        snprintf(buf, sizeof buf, "Thread");
        name_ = buf;
    }
}
//pthread_create����
void* startThread(void* obj)
{
    //�Ƿ���Ҫ����ָ���ֹ�ڴ�й©
    ThreadData* data = static_cast<ThreadData*>(obj);
    data->runInThread();
    delete data;
    return NULL;
}

void Thread::start()
{
    assert(!started_);
    started_ = true;
    ThreadData* data=new ThreadData(func_,name_,&tid_,&latch_);
    if(pthread_create(&pthreadId_,NULL,&startThread,data))
    {
        started_ = false;
        delete data;
    }else
    {
        latch_.wait();
        assert(tid_>0);
    }
}


int Thread::join()
{
    assert(started_);
    assert(!joined_);
    joined_ = true;
    return pthread_join(pthreadId_,NULL);
}
