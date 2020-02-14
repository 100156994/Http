

#include<stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

namespace socket
{

int createNonblockingOrDie(sa_family_t family)
{
#if VALGRIND
    int sockfd = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0)
    {
        //LOG_SYSFATAL << "sockets::createNonblockingOrDie";
        printf("sockets::createNonblockingOrDie\n")
    }

    setNonBlockAndCloseOnExec(sockfd);
#else
    int sockfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        //LOG_SYSFATAL << "sockets::createNonblockingOrDie";
        printf("sockets::createNonblockingOrDie \n");
    }
#endif
    return sockfd;
}

struct sockaddr_in getLocalAddr(int sockfd)
{
  struct sockaddr_in localaddr;
  memset(&localaddr,0,sizeof(localaddr));
  socklen_t addrlen = static_cast<socklen_t>(sizeof(localaddr));
  if (::getsockname(sockfd, static_cast<sockaddr>(&localaddr), &addrlen) < 0)
  {
    ///LOG_SYSERR << "sockets::getLocalAddr";
    printf("sockets::getLocalAddr error\n");
  }
  return localaddr;
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
    struct sockaddr* addr =static_cast<sockaddr addr*>(addr_in);
    socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);
    int connfd = ::accept(sockfd, addr, &addrlen);
    setNonBlockAndCloseOnExec(connfd);
    if (connfd < 0)
    {
        //log error
        printf("accept error\n");
        //暂时错误应当忽略  致命错误和未知错误应当终止程序
//        int savedErrno = errno;
//        LOG_SYSERR << "Socket::accept";
//        switch (savedErrno)
//        {
//        case EAGAIN:
//        case ECONNABORTED:
//        case EINTR:
//        case EPROTO: // ???
//        case EPERM:
//        case EMFILE: // per-process lmit of open file desctiptor ???
//            // expected errors
//            errno = savedErrno;
//            break;
//        case EBADF:
//        case EFAULT:
//        case EINVAL:
//        case ENFILE:
//        case ENOBUFS:
//        case ENOMEM:
//        case ENOTSOCK:
//        case EOPNOTSUPP:
//            // unexpected errors
//            LOG_FATAL << "unexpected error of ::accept " << savedErrno;
//            break;
//        default:
//            LOG_FATAL << "unknown error of ::accept " << savedErrno;
//            break;
//        }
    }
    return connfd;
}

}

using namespace Socket;


Socket::~Socket()
{
    if (::close(sockFd_) < 0)
    {
        //LOG_SYSERR << "sockets::close";
        printf("sockets close error\n");
    }
}


bool Socket::getTcpInfo(struct tcp_info* tcpi) const
{
  socklen_t len = sizeof(*tcpi);
  memset(tcpi,0,len);
  return ::getsockopt(sockfd_, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
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
        //LOG_SYSFATAL << "sockets::bindOrDie";
        printf("sockets bind error\n");
    }
}

void Socket::listen()
{
    int ret =  socketsListen(sockFd_);
    if(ret < 0)
    {
        //LOG_SYSFATAL << "sockets::bindOrDie";
        printf("sockets listen error\n");
    }
}


int Socket::accept(InetAddress* peerAddr)
{
    struct sockaddr_int addr;
    memset(&addr,0,sizeof(addr));
    int connfd = socketAccept(sockfd_, &addr);
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
        //LOG_SYSERR << "sockets::shutdownWrite";
        printf("sockets::shutdownWrite\n")
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
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
               &optval, static_cast<socklen_t>(sizeof optval));
    // FIXME CHECK
}
void Socket::setReusePort(bool on)
{
#ifdef SO_REUSEPORT
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT,
                         &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0 && on)
    {
        //LOG_SYSERR << "SO_REUSEPORT failed.";
        printf("SO_REUSEPORT failed\n")
    }
#else
    if (on)
    {
        //LOG_ERROR << "SO_REUSEPORT is not supported.";
        printf("SO_REUSEPORT is not supported\n")
    }
#endif
}
void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE,
               &optval, static_cast<socklen_t>(sizeof optval));
    // FIXME CHECK
}
