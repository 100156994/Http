#pragma once

#include"noncopyable.h"
#include<string>
#include<vector>
#include<functional>
#include <memory>
using std::string;
using std::vector;
class EventLoopThread;
class EventLoop;


//loop线程池  轮训获取线程  
class EventLoopThreadPool : noncopyable
{
public:

    typedef std::function<void(EventLoop*)> ThreadInitCallback;

    EventLoopThreadPool(EventLoop* baseLoop, const string& nameArg);
    ~EventLoopThreadPool();

    //必须在 start之前调用
    void setThreadNum(int numThreads) { numThreads_ = numThreads; }

    void start(const ThreadInitCallback& cb = ThreadInitCallback());
    ///轮训获取loop
    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoops();

    bool started() const
    { return started_; }

    const string& name() const
    { return name_; }

private:
    EventLoop* baseLoop_;
    string name_;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};
