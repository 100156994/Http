#pragma once

#include"noncopyable.h"
#include"Callback.h"
#include<string>


using namespace std::string

class EventLoop;
class Acceptor;



class TcpServer :noncopyable
{
public:
    typedef std::function<void(EventLoop*)> ThreadInitCallback;
    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop* loop,const InetAddress& listenAddr,const string& nameArg,Option option = kNoReusePort);
    ~TcpServer();


    const string& ipPort() const { return ipPort_; }
    const string& name() const { return name_; }
    EventLoop* getLoop() const { return loop_; }

    //设置线程数量 在start之前调用  accept在loop中  然后得到conn分发给线程池
    //0代表默认状态 只有loop线程
    void setThreadNum(int numThreads);
    void setThreadInitCallback(const ThreadInitCallback& cb)
    {  threadInitCallback_ = cb; }
    //只能在start之后调用  线程池获取只能在一个线程调用
    std::shared_ptr<EventLoopThreadPool> threadPool()
    { return threadPool_; }

      /// Thread safe. 多线程调用只会调用一次 通过atomic_flag
    void start();

    //设置回调  不是线程安全的
    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }
private:
    ///不是线程安全 但是只会在loop线程执行
    void newConnection(int sockfd,const InetAddress& peerAddr);
    /// 线程安全
    void removeConnection(const TcpConnectionPtr& conn);
    /// 不是线程安全 但是只会在loop线程执行
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    typedef std::map<string, TcpConnectionPtr> ConnectionMap;

    EventLoop* loop_;
    const string name_;
    const string ipPort_;
    std::unique_ptr<Acceptor> acceptor_;
    //loop 线程池
    std::shared_ptr<EventLoopThreadPool> threadPool_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    ThreadInitCallback threadInitCallback_;

    std::atomic_flag started_;

    int nextConnId_;
    ConnectionMap connections_;
};
