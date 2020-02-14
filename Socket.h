#pragma once

#include"noncopyable.h"
#include"InetAddress.h"


class InetAddress;

class Socket : noncopyable
{
public:
    explicit Socket(int fd)
        :sockFd_(fd)
        {
        }

    ~Socket();//析构时关闭fd

      // return true if success.
    bool getTcpInfo(struct tcp_info*) const;
    bool getTcpInfoString(char* buf, int len) const;



    int fd()const{return sockFd_;}
    //abort if address in use
    void bindAddress(const InetAddress& addr);
    void listen();
    //成功返回0  失败返回-1 且不修改传入参数
    int accept(InetAddress* peerAddr);

    void shutdownWrite();

    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

private:
    const int sockFd_;
};


namespace socket
{
int createNonblockingOrDie(sa_family_t family);
struct sockaddr_in getLocalAddr(int sockfd);
}
