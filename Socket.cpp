
#include"Socket.h"
#include<stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include<netinet/in.h>
#include <fcntl.h>
#include<netinet/tcp.h>
#include<string.h>
#include"Logging.h"

namespace mysocket
{


int createNonblockingOrDie(sa_family_t family)
{
#if VALGRIND
    int sockfd = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0)
    {
        //LOG << "sockets::createNonblockingOrDie";
        //printf("sockets::createNonblockingOrDie\n")
    }

    setNonBlockAndCloseOnExec(sockfd);
#else
    int sockfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        //LOG << "sockets::createNonblockingOrDie";
        //printf("sockets::createNonblockingOrDie \n");
    }
#endif
    return sockfd;
}

struct sockaddr_in getLocalAddr(int sockfd)
{
    struct sockaddr_in localaddr;
    memset(&localaddr,0,sizeof(localaddr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof(localaddr));
    if (::getsockname(sockfd, reinterpret_cast<sockaddr*>(&localaddr), &addrlen) < 0)
    {
      //LOG<<"sockets::getLocalAddr error";
    }
    return localaddr;
}

struct sockaddr_in getPeerAddr(int sockfd)
{
    struct sockaddr_in peeraddr;
    memset(&peeraddr,0,sizeof(peeraddr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof(peeraddr));
    if (::getpeername(sockfd, reinterpret_cast<sockaddr*>(&peeraddr), &addrlen) < 0)
    {
      //LOG<<"sockets::getPeerAddr error";
    }
    return peeraddr;
}

bool isSelfConnect(int sockfd)
{
    struct sockaddr_in localaddr = getLocalAddr(sockfd);
    struct sockaddr_in peeraddr = getPeerAddr(sockfd);
    if (localaddr.sin_family == AF_INET)
    {
        return localaddr.sin_port == peeraddr.sin_port
        && localaddr.sin_addr.s_addr == peeraddr.sin_addr.s_addr;
    }
    return false;
}

int socketsListen(int sockfd)
{
    int ret = ::listen(sockfd, SOMAXCONN);
    return ret;
}

void setNonBlockAndCloseOnExec(int sockfd)
{
    // non-block
    int flags = ::fcntl(sockfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    int ret = ::fcntl(sockfd, F_SETFL, flags);
    // FIXME check

    // close-on-exec
    flags = ::fcntl(sockfd, F_GETFD, 0);
    flags |= FD_CLOEXEC;
    ret = ::fcntl(sockfd, F_SETFD, flags);
   // FIXME check


}


int socketAccept(int sockfd, sockaddr_in* addr_in)
{
    struct sockaddr* addr =reinterpret_cast<sockaddr*>(addr_in);
    socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);
#if VALGRIND || defined (NO_ACCEPT4)
  int connfd = ::accept(sockfd, addr, &addrlen);
  setNonBlockAndCloseOnExec(connfd);
#else
  int connfd = ::accept4(sockfd, addr,
                        &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
//int connfd = ::accept(sockfd, addr, &addrlen);
  //  setNonBlockAndCloseOnExec(connfd);
#endif

    
    
    if (connfd < 0)
    {
        //log error
	int savedErrno = errno;
        //LOG<<"accept error";
      	switch (savedErrno)
    	{
      	    case EAGAIN://没有连接
      	    case ECONNABORTED://连接以及断开
      	    case EINTR://中断
      	    case EPROTO: // 协议错误 ???
      	    case EPERM://防火墙错误
      	    case EMFILE: // 进程的文件描述符到限制 ???
        	// expected errors
        	errno = savedErrno;
        	break;
      	    case EBADF://不是打开的文件描述符
            case EFAULT://地址不是可写的用户地址
      	    case EINVAL://不是监听套接字或者地址长度错误 或flag错误
      	    case ENFILE://系统的文件描述符到限制
      	    case ENOBUFS://没有足够的自由内存 这通常是指套接口内存分配被限制，而不是指系统内存不足
      	    case ENOMEM://同上
      	    case ENOTSOCK://文件描述符不是一个SOCKET
      	    case EOPNOTSUPP://引用的套接口不是 SOCK_STREAM 类型的。
       	 	// unexpected errors
        	//LOG<< "FATAL unexpected error of ::accept " << savedErrno;
        	break;
      	   default:
        	//LOG<< "unknown error of ::accept " << savedErrno;
        	break;
    	}
     }
        
    return connfd;
}

}

using namespace mysocket;


Socket::~Socket()
{
    if (::close(sockFd_) < 0)
    {
        //LOG<< "sockets::close error";
        //printf("sockets close error\n");
    }
}


bool Socket::getTcpInfo(struct tcp_info* tcpi) const
{
    socklen_t len = sizeof(*tcpi);
    memset(tcpi,0,len);
    return ::getsockopt(sockFd_, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
}

bool Socket::getTcpInfoString(char* buf, int len) const
{
    struct tcp_info tcpi;
    bool ok = getTcpInfo(&tcpi);
    if (ok)
    {
        snprintf(buf, len, "unrecovered=%u "
             "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
             "lost=%u retrans=%u rtt=%u rttvar=%u "
             "sshthresh=%u cwnd=%u total_retrans=%u",
             tcpi.tcpi_retransmits,  // Number of unrecovered [RTO] timeouts
             tcpi.tcpi_rto,          // Retransmit timeout in usec
             tcpi.tcpi_ato,          // Predicted tick of soft clock in usec
             tcpi.tcpi_snd_mss,
             tcpi.tcpi_rcv_mss,
             tcpi.tcpi_lost,         // Lost packets
             tcpi.tcpi_retrans,      // Retransmitted packets out
             tcpi.tcpi_rtt,          // Smoothed round trip time in usec
             tcpi.tcpi_rttvar,       // Medium deviation
             tcpi.tcpi_snd_ssthresh,
             tcpi.tcpi_snd_cwnd,
             tcpi.tcpi_total_retrans);  // Total retransmits for entire connection

            //是否传递的buffer 太小会导致写满
    }
    return ok;
}


void Socket::bindAddress(const InetAddress& addr)
{
    int ret = ::bind(sockFd_, addr.getSockAddr(), static_cast<socklen_t>(sizeof(struct sockaddr_in)));
    if (ret < 0)
    {
        //LOG<<"FATAL sockets bind error";
    }
}

void Socket::listen()
{
    int ret =  socketsListen(sockFd_);
    if(ret < 0)
    {
        //LOG<<"FATAL sockets bind error";
    }
}


int Socket::accept(InetAddress* peerAddr)
{
    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    int connfd = socketAccept(sockFd_, &addr);
    if (connfd >= 0)
    {
        peerAddr->setSockAddrInet(addr);
    }
    return connfd;
}


void Socket::shutdownWrite()
{
    if (::shutdown(sockFd_, SHUT_WR) < 0)
    {
        //LOG<<"error sockets::shutdownWrite";
    }
}



void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockFd_, IPPROTO_TCP, TCP_NODELAY,
               &optval, static_cast<socklen_t>(sizeof optval));
    // FIXME CHECK
}
void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockFd_, SOL_SOCKET, SO_REUSEADDR,
               &optval, static_cast<socklen_t>(sizeof optval));
    // FIXME CHECK
}
void Socket::setReusePort(bool on)
{
#ifdef SO_REUSEPORT
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockFd_, SOL_SOCKET, SO_REUSEPORT,
                         &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0 && on)
    {
        //LOG<< "SO_REUSEPORT failed.";
        //printf("SO_REUSEPORT failed\n");
    }
#else
    if (on)
    {
        //LOG<< "SO_REUSEPORT is not supported.";
        //printf("SO_REUSEPORT is not supported\n");
    }
#endif
}
void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockFd_, SOL_SOCKET, SO_KEEPALIVE,
               &optval, static_cast<socklen_t>(sizeof optval));
    // CHECK
}
