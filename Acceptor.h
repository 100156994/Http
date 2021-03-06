#pragma once


#include<functional>

#include"noncopyable.h"
#include"Socket.h"
#include"Channel.h"

class EventLoop;
class InetAddress;


//负责创建监听套接字 接受连接  每个连接回调(fd,peerAddr)  默认reuseADDR
class Acceptor : noncopyable
{
public:
    typedef std::function<void (int fd,const InetAddress& )> NewConnectionCallback;

    Acceptor(EventLoop* loop,const InetAddress& addr, bool reuseport);
    ~Acceptor();//关闭为了上限打开的空文件

    void setNewConnectionCallback(const NewConnectionCallback& cb){ newConnectionCallback_=cb; }
    bool listening()const {return listening_;}

    void listen();

private:
    void handleRead();
    EventLoop* loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    bool listening_;
    NewConnectionCallback newConnectionCallback_;
    //打开一个空文件 防止套接字用尽 设置软上限
    int idleFd_;
};
