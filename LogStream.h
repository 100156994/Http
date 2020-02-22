#pragma once


#include"noncopyable.h"
#include<string.h>
#include<string>


const int kSmallBuffer = 4000;
const int kLargeBuffer = 1000 *4000;

template<int SIZE>
class FixedBuffer : noncopyable
{
public:
    FixedBuffer():cur_(data_){}
    ~FixedBuffer(){}

    //超出buffer大小就什么也不做
    void append(const char* buf,size_t len)
    {
        if(avail() > static_cast<int>(len))
        {
            memcpy(cur_,buf,len);
            cur_+=len;
        }
    }

    const char* data()const {return data_;}
    int length()const { return static_cast<int>(cur_-data_); }
    char * current(){return cur_;}
    int avail()const {return static_cast<int>(end()-cur_);}
    void add(size_t len){cur_+=len;}
    void reset(){cur_=data_;}
    void bzero(){memset(data_,0,sizeof(data_));}


private:
    const char* end()const {return data_+sizeof(data_);}

    char data_[SIZE];
    char* cur_;
};


class LogStream : noncopyable
{

public:
    //typedef FixedBuffer<kLargeBuffer> LogBuffer;
    typedef FixedBuffer<kSmallBuffer> LogBuffer;
    typedef LogStream self;
    //默认构造析沟

    self& operator<<(bool value)
    {
        buffer_.append( value ? "1":"0",1);
        return *this;
    }
    //整型
    self& operator<<(short);
    self& operator<<(unsigned short);
    self& operator<<(int);
    self& operator<<(unsigned int);
    self& operator<<(long);
    self& operator<<(unsigned long);
    self& operator<<(long long);
    self& operator<<(unsigned long long);

    //浮点
    self& operator<<(float v) {
        *this << static_cast<double>(v);
        return *this;
    }
    self& operator<<(double);
    self& operator<<(long double);

    //字符
    self& operator<<(char v)
    {
        buffer_.append(&v, 1);
        return *this;
    }

    self& operator<<(const char* str)
    {
        if (str)
            buffer_.append(str, strlen(str));
        else
            buffer_.append("(null)", 6);
        return *this;
    }

    self& operator<<(const unsigned char* str)
    {
        return operator<<(reinterpret_cast<const char*>(str));
    }

    self& operator<<(const std::string& v) {
        buffer_.append(v.c_str(), v.size());
        return *this;
    }

    void append(const char* data, int len) { buffer_.append(data, len); }
    const LogBuffer& buffer() const { return buffer_; }
    void resetBuffer() { buffer_.reset(); }

private:
    void staticCheck();

    template <typename T>
    void formatInteger(T);

    LogBuffer buffer_;
    static const int kMaxNumericSize =32;
};
