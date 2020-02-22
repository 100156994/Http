#include"LogFile.h"



#include<errno.h>
#include<string.h>
#include<time.h>
// AppendFile


AppendFile::AppendFile(std::string filename)
    :fp_(fopen(filename.c_str(),"ae"))
    {
        setbuffer(fp_,buffer_,sizeof(buffer_));
    }

AppendFile::~AppendFile()
{
    ::fclose(fp_);
}


void AppendFile::append(const char* logline,const size_t len)
{
    size_t n = this->write(logline,len);
    size_t remain = len - n;
    while(remain > 0)
    {
        size_t num = this->write(logline+n,len);
        if(num == 0)
        {
            int err =ferror(fp_);
            if(err)
            {
                fprintf(stderr,"AppendFile::append() failed %s\n",strerror(err));
            }
            break;
        }
        n += num;
        remain = len - n;
    }
}


void AppendFile::flush()
{
    fflush(fp_);
}

size_t AppendFile::write(const char* logline, size_t len)
{
    return fwrite_unlocked(logline, 1, len, fp_);
}


//LogFile


LogFile::LogFile(const string& basename,off_t rollSize,bool threadSafe ,int flushInterval,int checkEveryN )
    :basename_(basename),
    rollSize_(rollSize),
    mutex_(threadSafe?new MutexLock:nullptr),
    flushInterval_(flushInterval),
    checkEveryN_(checkEveryN),
    count_(0),
    startOfPeriod_(0),
    lastRoll_(0),
    lastFlush_(0)
    {
        rollFile();
    }


void LogFile::append(const char* logline,size_t len)
{
    if(mutex_)
    {
        MutexLockGuard lock(*mutex_);
        append_unlocked(logline, len);
    }else
    {
        append_unlocked(logline, len);
    }
}
void LogFile::flush()
{
    if(mutex_)
    {
        MutexLockGuard lock(*mutex_);
        file_->flush();
    }else
    {
        file_->flush();
    }

}
bool LogFile::rollFile()
{
    time_t now =0;
    string filename = getLogFileName(basename_,&now);
    time_t thisPeriod = now/KRollPerSeconds_ *KRollPerSeconds_;//除去多余的秒
    if(now >lastRoll_)
    {
        lastRoll_ = now;
        lastFlush_ = now;
        startOfPeriod_ = thisPeriod;
        file_.reset(new AppendFile(filename));
        return true;
    }
    return false;
}

//basename+时间+.log
string LogFile::getLogFileName(const string& basename, time_t* now)
{
    string filename;
    filename.reserve(basename.size()+64);
    filename = basename;

    //获取UTC时间
    char timebuf[32];
    struct tm tm;
    *now = time(NULL);
    gmtime_r(now, &tm);
    strftime(timebuf, sizeof(timebuf), "%Y%m%d-%H%M%S", &tm);
    filename += timebuf;
    filename +=".log";
    return filename;
}

void LogFile::append_unlocked(const char* logline, int len)
{
    file_->append(logline,len);
    if(file_->writtenBytes() > rollSize_)
    {
        rollFile();
    }else{
        ++count_;
        if(count_>=checkEveryN_)
        {
            count_ = 0;
            time_t now =::time(NULL);
            time_t thisPeriod = now/KRollPerSeconds_ *KRollPerSeconds_;//除去多余的秒
            if(thisPeriod != startOfPeriod_)//滚动
            {
                rollFile();
            }else if(now - lastFlush_ > flushInterval_)//刷新
            {
                lastFlush_ = now;
                file_->flush();
            }//其他情况仅仅需要添加
        }

    }
}
