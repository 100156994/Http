#pragma once

#include<pthread.h>




#include"CountDownLatch.h"
#include"noncopyable.h"
#include <functional>
#include<string>
using std::string;

class Thread : noncopyable{
public:
    typedef std::function<void()> ThreadFunc;
    explicit Thread(const ThreadFunc& cb,const string& name=string());
    ~Thread();

    void start();
    int join();

    bool started()const {return started_;}
    //bool joined_()const {return joined_;}
    pthread_t pthreadId()const {return pthreadId_;}
    pid_t tid()const {return tid_;}
    const string& name()const {return name_;}


private:
    void setDefaultName();
    bool started_;
    bool joined_;
    pthread_t pthreadId_;
    pid_t tid_;
    ThreadFunc func_;
    string name_;
    CountDownLatch latch_;

    //是否需要静态成员记录线程创建数
};
