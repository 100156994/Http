
#include"TcpConnnection.h"

#include"Channel.h"
#include"EventLoop.h"
#include"Socket.h"
#include<aseert.h>
#include <unistd.h>
#include<sys/socket.h>

namespace socket
{

int getSocketError(int sockfd)
{
  int optval;
  socklen_t optlen = static_cast<socklen_t>(sizeof optval);

  if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
  {
    return errno;
  }
  else
  {
    return optval;
  }
}

}

using namespace socket;

TcpConnection::TcpConnection(EventLoop* loop, const string& name, int sockfd, const InetAddress& localAddr, const InetAddress peerAddr)
    :loop_(loop),
     name_(name),
     state_(kConnecting),
     reading_(false),
     socket_(new Socket(sockfd)),
     channel_(new Channel(loop_,sockfd)),
     localAddr_(localAddr),
     peerAddr_(peerAddr),
     highWaterMark_(64*1024*1024)
     {
        //设置channel
        channel_->setReadCallback(std::bind(&TcpConnection::handleRead,this,_1));
        channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
        channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
        channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
     }


TcpConnection::~TcpConnection()
{
//  LOG_DEBUG << "TcpConnection::dtor[" <<  name_ << "] at " << this
//            << " fd=" << channel_->fd()
//            << " state=" << stateToString();
    printf("TcpConnection::dtor [%s]\n",name_.c_str());
    assert(state_ == kDisconnected);
}


bool TcpConnection::getTcpInfo(struct tcp_info* tcpi) const
{
  return socket_->getTcpInfo(tcpi);
}

string TcpConnection::getTcpInfoString() const
{
  char buf[1024];
  buf[0] = '\0';
  socket_->getTcpInfoString(buf, sizeof buf);
  return buf;
}


void TcpConnection::send(const void* data, int len)
{
  send(StringPiece(static_cast<const char*>(data), len));
}

void TcpConnection::send(const StringPiece& message)
{
  if (state_ == kConnected)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(message);
    }
    else
    {
      loop_->runInLoop(
          std::bind(&TcpConnection::sendInLoop,this, message.as_string()));
        //std::forward<string>(message)));
    }
  }
}


void TcpConnection::send(Buffer* buf)
{
  if (state_ == kConnected)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(buf->peek(), buf->readableBytes());
      buf->retrieveAll();
    }
    else
    {
      loop_->runInLoop(
          std::bind(&TcpConnection::sendInLoop,this,buf->retrieveAllAsString()));
        //std::forward<string>(message)));
    }
  }
}

void TcpConnection::sendInLoop(const StringPiece& message)
{
  sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop(const void* data, size_t len)
{
  loop_->assertInLoopThread();
  ssize_t nwrote = 0;
  size_t remaining = len;
  bool faultError = false;
  if (state_ == kDisconnected)
  {
    ///LOG_WARN << "disconnected, give up writing";
    printf("disconnected, give up writing\n");
    return;
  }
  // if no thing in output queue, try writing directly
  if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
  {
    nwrote = ::write(channel_->fd(), data, len);
    if (nwrote >= 0)
    {
      remaining = len - nwrote;
      if (remaining == 0 && writeCompleteCallback_)
      {
        loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
      }
    }
    else // nwrote < 0
    {
      nwrote = 0;
      if (errno != EWOULDBLOCK)
      {
        ///LOG_SYSERR << "TcpConnection::sendInLoop";
        printf("TcpConnection::sendInLoop error\n")
        if (errno == EPIPE || errno == ECONNRESET) // any others?
        {
          faultError = true;//对端没有关闭
        }
      }
    }
  }

  assert(remaining <= len);
  if (!faultError && remaining > 0)
  {
    size_t oldLen = outputBuffer_.readableBytes();
    if (oldLen + remaining >= highWaterMark_
        && oldLen < highWaterMark_
        && highWaterMarkCallback_)
    {
      loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
    }
    outputBuffer_.append(static_cast<const char*>(data)+nwrote, remaining);
    if (!channel_->isWriting())
    {
      channel_->enableWriting();
    }
  }
}


void TcpConnection::shutdown()
{
  // 同时调用 没加锁可能导致多次
  if (state_ == kConnected)
  {
    setState(kDisconnecting);
    loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, shared_from_this()));
  }
}

void TcpConnection::shutdownInLoop()
{
  loop_->assertInLoopThread();
  //还有没写完的就不会关闭
  if (!channel_->isWriting())
  {
    // we are not writing
    socket_->shutdownWrite();
  }
}



void TcpConnection::forceClose()
{
  if (state_ == kConnected || state_ == kDisconnecting)
  {
    setState(kDisconnecting);
    loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
  }
}


void TcpConnection::forceCloseInLoop()
{
  loop_->assertInLoopThread();
  if (state_ == kConnected || state_ == kDisconnecting)
  {
    handleClose();
  }
}


void TcpConnection::setTcpNoDelay(bool on)
{
  socket_->setTcpNoDelay(on);
}

void TcpConnection::startRead()
{
  loop_->runInLoop(std::bind(&TcpConnection::startReadInLoop, this));
}

void TcpConnection::startReadInLoop()
{
  loop_->assertInLoopThread();
  if (!reading_ || !channel_->isReading())
  {
    channel_->enableReading();
    reading_ = true;
  }
}

void TcpConnection::stopRead()
{
  loop_->runInLoop(std::bind(&TcpConnection::stopReadInLoop, this));
}

void TcpConnection::stopReadInLoop()
{
  loop_->assertInLoopThread();
  if (reading_ || channel_->isReading())
  {
    channel_->disableReading();
    reading_ = false;
  }
}



void TcpConnection::connectEstablished()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    loop_->assertInLoopThread();
    if(kconnected)
    {
        setState(kDisconnected);
        channel_->disableAll();
        ConnectionCallback(shared_from_this());
    }
    channel_->remove();
}


void TcpConnection::handleRead(size_t receiveTime)
{
    loop_->assertInLoopThread();
    int savedErrno =0;
    ssize_t n = inputBuffer_.readFd(socket_,&savedErrno);
    if(n>0){
        messageCallback_(shared_from_this(),&inputBuffer_,receiveTime);
    }else if(n == 0){
        handleClose();
    }else{
        errno = savedErrno;
        //log
        printf("TcpConnection::handleRead  error \n");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    loop_->assertInLoopThread();
    if(channel_->isWriting())
    {
        ssize_t n = sockets::write(channel_->fd(),outputBuffer_.peek(),outputBuffer_.readableBytes());
        if (n > 0)
        {//有写入
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {//完全写完
                channel_->disableWriting();
                if (writeCompleteCallback_)
                {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }else{
            ///LOG_SYSERR
            printf("TcpConnection::handleWrite\n");
        }
    }else
    {
        ///LOG_TRACE
        printf("Connection fd = %d is down, no more writing\n",channel_->fd());
    }
}


void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    ///log
    printf("fd = %d  state = %s",channel_->fd(),stateToString().c_str());
    assert(state_ == kConnected || state_ == kDisconnecting);
    // we don't close fd, leave it to dtor, so we can find leaks easily.
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    ///用户的回调
    connectionCallback_(guardThis);
    ///绑定到server的remove 删除server中的map
    CloseCallback(guardThis);
}


void TcpConnection::handleError()
{
    int err = sockets::getSocketError(channel_->fd());


    ///LOG_ERROR
    printf("TcpConnection::handleError [%s] - SO_ERROR = %d",name_.c_str(),err);
            // " " << strerror_tl(err);
}

const char* TcpConnection::stateToString() const
{
  switch (state_)
  {
    case kDisconnected:
      return "kDisconnected";
    case kConnecting:
      return "kConnecting";
    case kConnected:
      return "kConnected";
    case kDisconnecting:
      return "kDisconnecting";
    default:
      return "unknown state";
  }
}
