#pragma once


#include"noncopyable.h"
#include"InetAddress.h"
#include<functional>
#include<memory>

class Channel;
class EventLoop;

//向目标地址一直发起连连接直到成功或者停止 然后回调  失败后重试时间加倍
class Connector : noncopyable,public std::enable_shared_from_this<Connector>
{
public:
    typedef std::function<void(int sockfd)> NewConnectionCallback;

    Connector(EventLoop* loop,const InetAddress& serverAddr);
    ~Connector();

    //线程安全   
    void start();
    void stop();

    void setNewConnectionCallback(const NewConnectionCallback& cb) { newConnectionCallback_ = cb; }


    const InetAddress& serverAddress()const {return serverAddr_;}
private:
    enum States { kDisconnected, kConnecting, kConnected };
    static const int kMaxRetryDelayMs = 30*1000;
    static const int kInitRetryDelayMs = 500;


    void setState(States s) { state_ = s; }
    void startInLoop();
    void stopInLoop();
    void connectInLoop();
    void connecting(int sockfd);
    void handleWrite();
    void handleError();
    void retry(int sockfd);
    int removeAndResetChannel();
    void resetChannel();


    EventLoop* loop_;
    InetAddress serverAddr_;
    bool connect_;
    States state_;
    std::unique_ptr<Channel> channel_;
    NewConnectionCallback newConnectionCallback_;
    int retryDelayMs_;
};
