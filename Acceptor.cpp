
#include"Acceptor.h"

#include"EventLoop.h"
#include"InetAddress.h"
#include"Logging.h"
#include <unistd.h>
#include<fcntl.h>

using namespace mysocket;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& addr, bool reuseport)
    :loop_(loop),
     acceptSocket_(createNonblockingOrDie(addr.family())),
     acceptChannel_(loop_,acceptSocket_.fd()),
     listening_(false),
     idleFd_(::open("/dev/null",O_RDONLY | O_CLOEXEC))
     {
        acceptSocket_.setReuseAddr(true);
        acceptSocket_.setReusePort(reuseport);
        acceptSocket_.bindAddress(addr);
        acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));
     }


//channel的fd关闭由Socket控制  本身空文件fd自己控制 
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

//每次acceptFd可读只会accept一次  因为epoll为LT模式
void Acceptor::handleRead()
{
    loop_->assertInLoopThread();
    InetAddress peerAddr(0);
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd>=0)
    {
        if(newConnectionCallback_)
        {
	    //LOG<<"newConnectionCallback_  fd = "<<connfd;
            newConnectionCallback_(connfd,peerAddr);
        }else
        {
            if (::close(connfd) < 0)
            {
                ///LOG<< "sockets::close";
            }
        }
    }else
    {
    	//LOG<< "in Acceptor::handleRead "<<errno;
    	///连接达到上限 释放空文件 然后关闭连接 在打开空文件
    	if (errno == EMFILE)
    	{
    	  ::close(idleFd_);
    	  idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);
	  //LOG<<" Acceptor::handleRead fd = "<<idleFd_;
    	  ::close(idleFd_);
      	  idleFd_ = open("/dev/null", O_RDONLY | O_CLOEXEC);
    	}
       //忽略其他错误   致命错误已经写入日志
     }
}
