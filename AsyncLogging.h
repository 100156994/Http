#pragma once

#include"LogStream.h"
#include"noncopyable.h"
#include"MutexLock.h"
#include"Condition.h"
#include"CountDownLatch.h"
#include"Thread.h"
#include<string>
#include<memory>
#include<vector>

//双缓冲 前后端  前端负责往缓存写   后端（本身是一个线程）负责把缓冲写进磁盘  默认3秒向磁盘刷新一次
class AsyncLogging : noncopyable
{
public:
    AsyncLogging(const std::string basename,off_t rollSize,int flushInterval =3 );
    ~AsyncLogging()
    {
        if(running_) stop();
    }

    void start()
    {
        running_ = true;
        thread_.start();
        latch_.wait();
    }
    void stop()
    {
        running_ = false;
        cond_.notify();
        thread_.join();
    }

    void append(const char* logline,int len);

private:
    void threadFunc();
    typedef FixedBuffer<kLargeBuffer> LogBuffer;
    typedef std::vector<std::unique_ptr<LogBuffer>> BufferVector;
    typedef std::unique_ptr<LogBuffer> BufferPtr;

    std::string basename_;
    const int flushInterval_;
    bool running_;
    const off_t rollSize_;

    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
    CountDownLatch latch_;
    BufferPtr currentBuffer_;
    BufferPtr nextBuffer_;
    BufferVector buffers_;
};

