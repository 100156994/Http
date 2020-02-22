#include"Connector.h"

#include"EventLoop.h"
#include"Channel.h"
#include"Socket.h"
#include<errno.h>
#include"Logging.h"
#include"TcpConnection.h"

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop* loop,const InetAddress& serverAddr)
    :loop_(loop),
     serverAddr_(serverAddr),
     connect_(false),
     state_(kDisconnected),
     retryDelayMs_(kInitRetryDelayMs)
    {
        LOG<<"Connector construct [ "<<this<<" ]";
    }

Connector::~Connector()
{
    LOG<<"Connector deconstruct [ "<<this<<" ]";
}


void Connector::start()
{
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop,this)); //调用之前 connetor析构 是否需要用shared_ptr
}

void Connector::stop()
{
    connect_ = false;
    loop_->queueInLoop(std::bind(&Connector::stopInLoop, this));
}

void Connector::startInLoop()
{
    loop_->assertInLoopThread();
    assert(state_ == kDisconnected);
    if (connect_)
    {
        connectInLoop();
    }
    else
    {
        LOG<< "do not connect";
    }

}

void Connector::connectInLoop()
{
	LOG<<"connenctInloop";
    int sockfd = mysocket::createNonblockingOrDie(serverAddr_.family());
    int ret =::connect(sockfd, serverAddr_.getSockAddr(), static_cast<socklen_t>(sizeof(struct sockaddr)));
    int savedErrno = (ret==0)?0:errno;
    switch(savedErrno)
    {
        case 0:
        case EINPROGRESS://套接字为非阻塞套接字，且连接请求没有立即完成。
        case EINTR://中断
        case EISCONN://已经连接到该套接字
            connecting(sockfd);
            break;

        case EAGAIN: //路由缓存不足
        case EADDRINUSE://本地地址处于使用状态
        case EADDRNOTAVAIL://自动bind的的端口全部暂时不可用
        case ECONNREFUSED://远程地址并没有处于监听状态
        case ENETUNREACH:
            retry(sockfd);
            break;

        case EACCES://用户试图在套接字广播标志没有设置的情况下连接广播地址或由于防火墙策略导致连接失败
        case EPERM://用户试图在套接字广播标志没有设置的情况下连接广播地址或由于防火墙策略导致连接失败
        case EAFNOSUPPORT://地址不符合正确的地址簇
        case EALREADY://套接字为非阻塞套接字，并且原来的连接请求还未完成
        case EBADF://不是一个打开的文件描述符
        case EFAULT:// 指向套接字结构体的地址非法
        case ENOTSOCK://文件描述符不与套接字相关
            LOG<< "connect error in Connector::connectInLoop " << savedErrno;
            ::close(sockfd);
            break;

        default:
            LOG<< "Unexpected error in Connector::connectInLoop " << savedErrno;
            ::close(sockfd);
            // connectErrorCallback_();
      break;
    }
}
//设置channel回调 开启写
void Connector::connecting(int sockfd)
{

    setState(kConnecting);
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallback(std::bind(&Connector::handleWrite, this));
    channel_->setErrorCallback(std::bind(&Connector::handleError, this));
    channel_->enableWriting();
}


void Connector::retry(int sockfd)
{
    ::close(sockfd);
    setState(kDisconnected);
    if (connect_)
    {
        LOG<< "Connector::retry - Retry connecting to " << serverAddr_.toIpPort()
             << " in " << retryDelayMs_ << " milliseconds. ";
        loop_->runAfter(retryDelayMs_/1000.0,
                    std::bind(&Connector::startInLoop, shared_from_this()));
        retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
    }
    else
    {
        LOG<< "do not connect";
    }
}



void Connector::stopInLoop()
{
    loop_->assertInLoopThread();
    if (state_ == kConnecting)
    {
        setState(kDisconnected);
        int sockfd = removeAndResetChannel();
        ::close(sockfd);
    }
}

int Connector::removeAndResetChannel()
{
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();
  //有可能在handelEvent 不能至今析构chanel
    loop_->queueInLoop(std::bind(&Connector::resetChannel, this));
    return sockfd;
}

void Connector::resetChannel()
{
    channel_.reset();
}


namespace mysocket
{
    int getSocketError(int sockfd);
    bool isSelfConnect(int sockfd);
}

void Connector::handleError()
{
    LOG<< "Connector::handleError state=" << state_;
    if (state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = mysocket::getSocketError(sockfd) ;
        LOG<< "SO_ERROR = " << err << " " << strerror(err);
        retry(sockfd);
    }
}


void Connector::handleWrite()
{
    LOG<< "Connector::handleWrite " << state_;

    if (state_ == kConnecting)//设置好channel 并且可写了 如果没有错误说明连接建立成功  Connector返回sockfd 不需要拥有channel
    {
        int sockfd = removeAndResetChannel();
        int err = mysocket::getSocketError(sockfd);
        if (err)
        {
            LOG<< "Connector::handleWrite - SO_ERROR = "<< err << " " << strerror(err);
            retry(sockfd);
        }
        else if (mysocket::isSelfConnect(sockfd))
        {
            LOG<< "Connector::handleWrite - Self connect";
            retry(sockfd);
        }
        else
        {
            setState(kConnected);
            if (connect_)
            {
                newConnectionCallback_(sockfd);
            }
            else
            {
                ::close(sockfd);
            }
        }
    }
    else
    {
        //理论上不会走到这个分支 在设置channel之前会设置状态  状态的设置只会在loop线程  切stop后会关闭chennl的写监听
        assert(state_ == kDisconnected);
    }
}
