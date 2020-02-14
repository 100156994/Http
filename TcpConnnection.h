#pragma once

#include<memory>
#include<string>

#include"InetAddress.h"
#include"noncopyable.h"
#include"Callback.h"
#include"Buffer.h"

// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

class Socket;
class Channel;
class EventLoop;


using std::string;

class TcpConnection;

class TcpConnection : noncopyable,
                      public std::enable_shared_from_this<HttpData>
{
public:
    TcpConnection(EventLoop* loop,const string& name,int sockfd,const InetAddress& localAddr,const InetAddress peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; }
    const string name() const { return name_; }
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }
    bool connected() const { return state_ == kConnected; }
    bool disconnected() const { return state_ == kDisconnected; }

      // 成功 return true
    bool getTcpInfo(struct tcp_info*) const;
    string getTcpInfoString() const;

    void send(const void* message, int len);
    void send(const StringPiece& message);
    void send(Buffer* message);  // this one will swap data
    void shutdown(); // 非线程安全  没有加锁保护状态 同时调用可能导致shutdownInloop 调用多次
    ///是否可以shutdown多次 需要测试

    void forceClose();
    //void forceCloseWithDelay(double seconds);
    void setTcpNoDelay(bool on);
    // reading or not
    void startRead();
    void stopRead();
    bool isReading() const { return reading_; }; // 不是线程安全的


    //设置回调
    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }

    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
    { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }

    /// 仅在内部使用 通知server移除所有connptr   用户回调使用connectionCallback
    void setCloseCallback(const CloseCallback& cb)
    { closeCallback_ = cb; }

      /// Advanced interface
    Buffer* inputBuffer()
    { return &inputBuffer_; }

    Buffer* outputBuffer()
    { return &outputBuffer_; }

    // called when TcpServer accepts a new connection
    void connectEstablished();   // should be called only once
    // called when TcpServer has removed me from its map
    void connectDestroyed();  // should be called only once
private:
    enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
    ///channel 绑定的回调
    void handleRead(size_t receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    /// loop线程进行真正的发送和关闭
    void sendInLoop(const StringPiece& message);
    void sendInLoop(const void* message, size_t len);
    void shutdownInLoop();
    void forceCloseInLoop();
    void startReadInLoop();
    void stopReadInLoop();
    ///状态
    void setState(StateE s) { state_ = s; }
    const char* stateToString() const;

    EventLoop* loop_;
    const string name_;
    StateE state_;
    bool reading_;
    //internal
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    const InetAddress localAddr_;
    const InetAddress peerAddr_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;
    size_t highWaterMark_;
    Buffer inputBuffer_;
    Buffer outputBuffer_;
};

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
