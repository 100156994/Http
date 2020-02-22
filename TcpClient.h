#pragma once

#include"noncopyable.h"
#include"Connector.h"
#include"Callback.h"
#include"Callback.h"
#include"MutexLock.h"
class Connector;
typedef std::shared_ptr<Connector> ConnectorPtr;


//用Connector来创建连接  对connPtr加锁来保护
class TcpClient : noncopyable
{
public:
    TcpClient(EventLoop* loop,const InetAddress& serverAddr,const string& nameArg);
    ~TcpClient(); 

    void connect();
    void disconnect();
    void stop();

    TcpConnectionPtr connection() const
    {
        MutexLockGuard lock(mutex_);
        return connection_;
    }

    EventLoop* getLoop() const { return loop_; }
    const string& name() const{ return name_; }

    /// Set ccallback.
    /// Not thread safe.
    void setConnectionCallback(ConnectionCallback cb){ connectionCallback_ = std::move(cb); }

    void setMessageCallback(MessageCallback cb){ messageCallback_ = std::move(cb); }

    void setWriteCompleteCallback(WriteCompleteCallback cb){ writeCompleteCallback_ = std::move(cb); }


private:
    void newConnection(int sockfd);

    void removeConnection(const TcpConnectionPtr& conn);


    EventLoop* loop_;
    ConnectorPtr connector_;
    const string name_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    bool connect_;
    // always in loop thread
    int nextConnId_;
    mutable MutexLock mutex_;
    TcpConnectionPtr connection_;

};
