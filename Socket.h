#pragma once

#include"noncopyable.h"



class Socket : noncopyable
{
public:
    explicit Socket(int fd)
        :sockFd_(fd)
        {
        }

    ~Socket();//¸ºÔð¹Ø±Õfd

    int fd()const{return sockFd_;}



private:
    const int sockFd_;
};
