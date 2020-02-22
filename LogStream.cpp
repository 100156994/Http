#include"LogStream.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include<algorithm>

const char digits[] = "9876543210123456789";
const char* zero = digits + 9;

//muduo 利用一个数组来对负数处理
template <typename T>
size_t convert(char buf[], T value)
{
    T i = value;
    char* p = buf;
    do
    {
        int lsd = static_cast<int>(i % 10);
        i /= 10;
        *p++ = zero[lsd];
    }
    while (i != 0);

    if (value < 0)
    {
        *p++ = '-';
    }
    *p = '\0';
    std::reverse(buf, p);
    return p-buf;
}
//显示模板特化
template class FixedBuffer<kSmallBuffer>;
template class FixedBuffer<kLargeBuffer>;


template <typename T>
void LogStream::formatInteger(T v)
{
    // buffer容不下kMaxNumericSize个字符的话会被直接丢弃 个人感觉不会出现一条日志把buffer写满 基本上不会容不下
    if (buffer_.avail() >= LogStream::kMaxNumericSize)
    {
        size_t len = convert(buffer_.current(), v);//这里已经把buff的内容改变
        buffer_.add(len);
    }
}

LogStream& LogStream::operator<<(short value)
{
    *this <<static_cast<int>(value);
    return *this;
}

LogStream& LogStream::operator<<(unsigned short value)
{
    *this <<static_cast<unsigned int>(value);
    return *this;
}

LogStream& LogStream::operator<<(int value)
{
    formatInteger(value);
    return *this;
}
LogStream& LogStream::operator<<(unsigned int value)
{
    formatInteger(value);
    return *this;
}
LogStream& LogStream::operator<<(long value)
{
    formatInteger(value);
    return *this;
}
LogStream& LogStream::operator<<(unsigned long value)
{
    formatInteger(value);
    return *this;
}
LogStream& LogStream::operator<<(long long value)
{
    formatInteger(value);
    return *this;
}
LogStream& LogStream::operator<<(unsigned long long value)
{
    formatInteger(value);
    return *this;
}


LogStream& LogStream::operator<<(double value)
{
    if (buffer_.avail() >= kMaxNumericSize)
    {
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", value);
        buffer_.add(len);
    }
    return *this;
}


LogStream& LogStream::operator<<(long double value)
{
    if (buffer_.avail() >= kMaxNumericSize) {
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12Lg", value);
        buffer_.add(len);
    }
    return *this;
}


