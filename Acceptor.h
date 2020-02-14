#pragma once


#include<functional>

#include"noncopyable.h"
#include"Socket.h"
#include"Channel.h"

class EventLoop;
class InetAddress;

class Acceptor : noncopyable
{
public:
    typedef std::function<void (int fd,const InetAddress& )> NewConnectionCallback;

    Acceptor(EventLoop* loop,const InetAddress* addr, bool reuseport);
    ~Acceptor();//�ر�Ϊ�����޴򿪵Ŀ��ļ�

    void setNewConnectionCallback(const NewConnectionCallback& cb){ newConnectionCallback_=cb; }
    bool listening()const {return listen;}

    void listen();

private:
    void handleRead();
    EventLoop* loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    bool listening_;
    NewConnectionCallback newConnectionCallback_;
    //��һ�����ļ� ��ֹ�׽����þ� ����������
    int idleFd_;
};
