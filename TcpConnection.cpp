
#include"TcpConnection.h"

#include"Channel.h"
#include"EventLoop.h"
#include"Socket.h"
#include"Logging.h"
#include<assert.h>
#include <unistd.h>
#include<sys/socket.h>
#include<stdio.h>

namespace mysocket
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

using namespace mysocket;

void defaultConnectionCallback(const TcpConnectionPtr& conn)
{
    //LOG<< conn->localAddress().toIpPort() << " -> "<< conn->peerAddress().toIpPort() << " is "<< (conn->connected() ? "UP" : "DOWN");
}

void defaultMessageCallback(const TcpConnectionPtr&,Buffer* buf,size_t time)
{
    buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop* loop, const string& name, int sockfd, const InetAddress& localAddr, const InetAddress peerAddr)
    :loop_(loop),
     name_(name),
     state_(kConnecting),
     reading_(false),
     socket_(new Socket(sockfd)),
     channel_(new Channel(loop_,sockfd)),
     localAddr_(localAddr),
     peerAddr_(peerAddr),
     connectionCallback_(defaultConnectionCallback),
     messageCallback_(defaultMessageCallback),
     highWaterMark_(64*1024*1024)
     {
        //设置channel
        channel_->setReadCallback(std::bind(&TcpConnection::handleRead,this,_1));
        channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
        channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
        channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
	socket_->setKeepAlive(true);
     }


TcpConnection::~TcpConnection()
{
    //LOG << "TcpConnection::dtor[" <<  name_ << "] at " << this
        //<< " fd=" << channel_->fd()<< " state=" << stateToString();
    //printf("TcpConnection::dtor [%s]  %s\n",name_.c_str(),stateToString());
    //主动关闭连接shutdown 后如果客户析构会导致状态在disconnting
    //printf("~TcpConnection\n");
    assert(state_ == kDisconnected||state_==kDisconnecting);
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
    if (loop_->isLoopInThread())
    {
      sendInLoop(message);
    }
    else
    {
      loop_->runInLoop(
          std::bind(&TcpConnection::sendInLoop,this, message));
        //std::forward<string>(message)));
    }
  }
}


void TcpConnection::send(Buffer* buf)
{
  if (state_ == kConnected)
  {
    if (loop_->isLoopInThread())
    {
      sendInLoop_1(buf->peek(), buf->readableBytes());
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
  sendInLoop_1(message.data(), message.size());
}

void TcpConnection::sendInLoop_1(const void* data, size_t len)
{
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    if (state_ == kDisconnected)
    {
      //LOG<< "disconnected, give up writing";
      //printf("disconnected, give up writing\n");
      return;
    }
    // 输出buffer没有待输出内容 
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
    else // nwrote < 0 写发生错误
    {
      	nwrote = 0;
     	if (errno != EWOULDBLOCK)
      	{
            //LOG<< "TcpConnection::sendInLoop error";
            //printf("TcpConnection::sendInLoop error\n");
            if (errno == EPIPE || errno == ECONNRESET) // 是否有其他错误?
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
    	if (oldLen + remaining >= highWaterMark_&& oldLen < highWaterMark_&& highWaterMarkCallback_)//如果缓存内容太多 高水位回调
    	{
     	    loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
    	}
    	outputBuffer_.append(static_cast<const char*>(data)+nwrote, remaining);
   	if (!channel_->isWriting())//开启channel监听写事件
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
	//printf("shutdown\n");
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, shared_from_this()));
    }
}

void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    //还有没写完的就不会关闭
    if (!channel_->isWriting())
    {
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
    //printf("conn established\n");
    connectionCallback_(shared_from_this());
}

//如果用户不持有ptr 那么这个函数调用后TcpConnection析构
void TcpConnection::connectDestroyed()
{
    LOG<<"TcpConnection connectDestroyed";
    loop_->assertInLoopThread();
    if(kConnected)
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
    ssize_t n = inputBuffer_.readFd(socket_->fd(),&savedErrno);
    if(n>0){
	//printf("messageCallback  %zd  \n",inputBuffer_.readableBytes());

        messageCallback_(shared_from_this(),&inputBuffer_,receiveTime);
    }else if(n == 0){
	//
	//printf("read 0\n");
        handleClose();

    }else{
        errno = savedErrno;
        //LOG<<"TcpConnection::handleRead  error";
        //printf("TcpConnection::handleRead  error \n");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    //printf("conn write\n");
    loop_->assertInLoopThread();
    if(channel_->isWriting())
    {
        ssize_t n = ::write(channel_->fd(),outputBuffer_.peek(),outputBuffer_.readableBytes());
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
            //LOG<<"TcpConnection::handleWrite error";
            //printf("TcpConnection::handleWrite\n");
        }
    }else
    {
        //LOG<<"Connection fd = "<<channel_->fd()<<"is down, no more writing";
        //printf("Connection fd = %d is down, no more writing\n",channel_->fd());
    }
}


void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    //LOG<<"fd= "<<channel_->fd()<<" state = "<<this->stateToString();
    
    //printf("fd = %d  state = %s\n",channel_->fd(),stateToString());
    assert(state_ == kConnected || state_ == kDisconnecting);
    // fd 在socket析构时关闭
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    //用户的回调
    connectionCallback_(guardThis);
    //绑定到server的remove 删除server中的map

    closeCallback_(guardThis);
}


void TcpConnection::handleError()
{
    int err = mysocket::getSocketError(channel_->fd());


    //LOG<<"TcpConnection::handleError ["<<name_.c_str()<<"]  - SO_ERROR ="<<err;
    //printf("TcpConnection::handleError [%s] - SO_ERROR = %d",name_.c_str(),err);
  
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
