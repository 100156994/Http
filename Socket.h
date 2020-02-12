#pragma once

#include"noncopyable.h"



class Socket : noncopyable
{
public:
    explicit Socket(int fd)
        :sockFd_(fd)
        {
        }

    ~Socket();//����ر�fd

    int fd()const{return sockFd_;}



private:
    const int sockFd_;
};
