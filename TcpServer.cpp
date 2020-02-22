
#include"TcpServer.h"
#include"EventLoop.h"
#include"Acceptor.h"
#include"InetAddress.h"
#include"Socket.h"
#include<stdio.h>
#include"TcpConnection.h"
#include"Logging.h"
using namespace mysocket;

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr, const string& nameArg, Option option)
    :loop_(loop),
     name_(nameArg),
     ipPort_(listenAddr.toIpPort()),
     acceptor_(new Acceptor(loop_,listenAddr,option==kReusePort)),
     threadPool_(new EventLoopThreadPool(loop, name_)),
     started_(ATOMIC_FLAG_INIT),
     nextConnId_(1)
{
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection,this,_1,_2));
}


//
TcpServer::~TcpServer()
{
    loop_->assertInLoopThread();

    //LOG << "TcpServer::~TcpServer [" << name_ << "] destructing";
    //printf("TcpServer::~TcpServer\n");

    for (auto& item : connections_)
    {
        TcpConnectionPtr conn(item.second);
        item.second.reset();//释放map里的coonptr
        conn->getLoop()->runInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn));//在conn对应loop池里线程里 摧毁连接 
    }
}

void TcpServer::setThreadNum(int numThreads)
{
  assert(0 <= numThreads);
  threadPool_->setThreadNum(numThreads);
}


void TcpServer::start()
{
  if (!started_.test_and_set())
  {
    threadPool_->start(threadInitCallback_);

    assert(!acceptor_->listening());
    loop_->runInLoop(
        std::bind(&Acceptor::listen, acceptor_.get()));
  }
}


void TcpServer::newConnection(int sockfd,const InetAddress& peerAddr)
{
    loop_->assertInLoopThread();
    EventLoop* ioLoop = threadPool_->getNextLoop();
    char buf[64];
    snprintf(buf,sizeof(buf),"#%d",nextConnId_);
    ++nextConnId_;
    string connName = name_+buf;

    InetAddress localAddr(mysocket::getLocalAddr(sockfd));
    
    //LOG<<"TcpServer::newConnection ["<<name_.c_str()<<"] - new connection ["<<connName.c_str()<<"] from "<<peerAddr.toIpPort().c_str();
    
    //printf("TcpServer::newConnection [%s] - new connection [%s] from %s\n",name_.c_str(),connName.c_str(),peerAddr.toIpPort().c_str());
    TcpConnectionPtr conn(new TcpConnection(ioLoop,connName,sockfd,localAddr,peerAddr));
    connections_[connName] = conn;

    conn->setTcpNoDelay(true);     

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, _1));
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}


void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop,this,conn));
}


void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    
    //LOG<<"TcpServer::removeConnectionInLoop ["<<name_.c_str()<<"] - connection ["<<conn->name().c_str()<<"]";
    //printf("TcpServer::removeConnectionInLoop [%s] - connection %s\n",name_.c_str(),conn->name().c_str());
    size_t n = connections_.erase(conn->name());
    assert(n ==1);
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed,conn));
}
