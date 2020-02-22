
#pragma once

#include"noncopyable.h"
#include"MutexLock.h"
#include<string>
#include<stdio.h>
#include <sys/types.h> //off_t
#include<memory> //unique_ptr

using std::string;
//为文件设置缓存 以追加的方式打开  
class AppendFile : noncopyable
{
public:
    explicit AppendFile(string filename);
    ~AppendFile();

    void append(const char* logline,const size_t len);

    void flush();

    off_t writtenBytes()const{return writtenBytes_;}

private:
    size_t write(const char* logline,const size_t len);

    FILE* fp_;
    char buffer_[64*1024];
    off_t writtenBytes_;

};


//负责日志文件的刷新 滚动  可以加锁 默认不加 只有一个后台线程写文件
class LogFile : noncopyable
{
public:
    LogFile(const string& basename,off_t rollSize,bool threadSafe = true,int flushInterval = 3,int checkEveryN = 1024);

    ~LogFile()=default;

    void append(const char* logline,size_t len);
    void flush();
    bool rollFile();

private:
    static string getLogFileName(const string& basename, time_t* now);

    void append_unlocked(const char* logline, int len);



    const string basename_;
    const off_t rollSize_;
    std::unique_ptr<MutexLock> mutex_;

    const int flushInterval_;
    const int checkEveryN_;
    int count_;
    time_t startOfPeriod_;
    time_t lastRoll_;
    time_t lastFlush_;
    std::unique_ptr<AppendFile> file_;
    const static int KRollPerSeconds_ = 24*60*60;
};
