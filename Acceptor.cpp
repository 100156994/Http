
#include"Acceptor.h"

#include"EventLoop.h"
#include"InetAddress.h"

#include <unistd.h>
#include<fcntl.h>

using namespace socket;

Acceptor::Acceptor(EventLoop* loop, const InetAddress* addr, bool reuseport)
    :loop_(loop),
     acceptSocket_(createNonblockingOrDie(addr->family)),
     acceptChannel_(acceptSocket_.fd()),
     listening_(false),
     idleFd_(::open("/dev/null",O_RDONLY | O_CLOEXEC))
     {
        acceptSocket_.setReuseAddr(true);
        acceptSocket_.setReusePort(reuseport);
        acceptSocket_.bindAddress(addr);
        acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));
     }

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    close(idleFd_);
}


void Acceptor::listen()
{
    loop_->assertInLoopThread();
    listening_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}


void Acceptor::handleRead()
{
    loop_->assertInLoopThread();
    InetAddress peerAddr(0);
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd>=0)
    {
        if(newConnectionCallback_)
        {
            newConnectionCallback_(connfd,peerAddr);
        }else
        {
            if (::close(sockfd) < 0)
            {
                //LOG_SYSERR << "sockets::close";
                printf("sockets::close\n");
            }
        }
    }
}
