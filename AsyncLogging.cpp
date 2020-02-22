
#include"AsyncLogging.h"
#include<assert.h>
#include"LogFile.h"


AsyncLogging::AsyncLogging(const std::string basename,off_t rollSize,int flushInterval)
    :basename_(basename),
     flushInterval_(flushInterval),
     running_(false),
     rollSize_(rollSize),
     thread_(std::bind(&AsyncLogging::threadFunc,this)),
     mutex_(),
     cond_(mutex_),
     latch_(1),
     currentBuffer_(new LogBuffer),
     nextBuffer_(new LogBuffer),
     buffers_()
     {
        assert(basename.size()>0);
        currentBuffer_->bzero();
        nextBuffer_->bzero();
        buffers_.reserve(16);
     }

//前端线程调用  
void AsyncLogging::append(const char* logline,int len)
{
    MutexLockGuard lock(mutex_);
    if(currentBuffer_->avail() > len)
    {
        currentBuffer_->append(logline,len);
    }else
    {
        buffers_.push_back(std::move(currentBuffer_));
        if(nextBuffer_)
        {
            currentBuffer_ = std::move(nextBuffer_);
        }else
        {
            currentBuffer_.reset(new LogBuffer);
        }
        currentBuffer_->append(logline,len);
        cond_.notify();
    }
}



void AsyncLogging::threadFunc()
{
    assert(running_==true);
    latch_.countDown();//后台线程已经准备好 令start返回
    LogFile output(basename_,rollSize_,false,flushInterval_);
    BufferPtr newBuffer1(new LogBuffer);
    BufferPtr newBuffer2(new LogBuffer);
    newBuffer1->bzero();
    newBuffer2->bzero();
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);
    while(running_)
    {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());
	//printf("handle buffer\n");
        {//交换Buffer 需要加锁
            MutexLockGuard lock(mutex_);
            if(buffers_.empty())
                cond_.waitForSeconds(flushInterval_);
            buffers_.push_back(std::move(currentBuffer_));
            currentBuffer_ = std::move(newBuffer1);
            buffersToWrite.swap(buffers_);
            if(!nextBuffer_)
                nextBuffer_ = std::move(newBuffer2);
        }
        assert(!buffersToWrite.empty());
	//往磁盘写 以及获取到缓冲内容  释放锁
        if(buffersToWrite.size()>25)//buffers 太多会忽略除了前两个buffer以外日志
        {
            char buf[256];
            snprintf(buf, sizeof buf, "Dropped log messages , %zd larger buffers\n",buffersToWrite.size()-2);
            output.append(buf, static_cast<int>(strlen(buf)));
            buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end());
        }

         for (const auto& buffer : buffersToWrite)
        {
            output.append(buffer->data(), buffer->length());
        }

         if (buffersToWrite.size() > 2)
        {
            buffersToWrite.resize(2);
        }
        if (!newBuffer1)
        {
            assert(!buffersToWrite.empty());
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }

        if (!newBuffer2)
        {
            assert(!buffersToWrite.empty());
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear();
        output.flush();
    }

}
