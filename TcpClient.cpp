
#include"TcpClient.h"
#include"Logging.h"
#include"EventLoop.h"
#include"TcpConnection.h"
#include"Socket.h"
#include"InetAddress.h"

namespace detail
{
void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn)
{
  loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}
}



//设置默认回调 或者加if 70行（有bug）

TcpClient::TcpClient(EventLoop* loop,const InetAddress& serverAddr,const string& nameArg)
  : loop_(loop),
    connector_(new Connector(loop, serverAddr)),
    name_(nameArg),
    connectionCallback_(defaultConnectionCallback),
    messageCallback_(defaultMessageCallback),
    connect_(true),
    nextConnId_(1)
    {
        connector_->setNewConnectionCallback(std::bind(&TcpClient::newConnection, this, _1));
        LOG << "TcpClient::TcpClient[" << name_ << "] - connector " << connector_.get();
    }

//完成连接的关闭工作
TcpClient::~TcpClient()
{
    LOG<<"TcpClient destruct";
    TcpConnectionPtr conn;
    bool unique = false;
    {
    	MutexLockGuard lock(mutex_);
	//printf("try unique %d\n",connection_.use_count());
    	unique = connection_.unique();
    	conn = connection_;
    }
    if (conn)//建立了连接 如果没有连接 停止尝试 不然会导致fd泄漏
    {
    	assert(loop_ == conn->getLoop());
    	// FIXME: not 100% safe, if we are in different thread
	// loop线程会回调 removeConn 所以在析构时要改变关闭回调  所以用户调用disconnect只能关闭写端 
	//否则会导致loop的func 摧毁conn在前 这样回调removeConn已经析构   因为setClose还在排队  
	//如果其他地方以及调用了forceclose client再析构 也会出现上述问题
	//还有问题 当调用disconnect后立即析构  这时有两个ptr 一个用来shutdown一个用来设置关闭回调  所以析构时不会强制关闭
	//当dofunc时 先关闭写段，这时候应该期待服务器响应然后调用close回调来销毁连接  但是设置完关闭回调后已经没有了ptr 直接析构
	//会触发channel addToLoop 断言  因为channel析构时不会向loop删除自己  这样epoll的指针就一直不会删除
	//--不过实际影响好像不大，下次新建时只是会被当作kDelted的channel 以及在debug时触发channel析构的断言  
	//且fd随着Socket析构关闭 也不会影响epoll的状态  只是map里多了一个不存在的channel指针 

	//如果在disconnect时先设置回调 然后直接强制关闭  这样可以保证关闭   但是这样析构时如果没有用户指针 可以直接强制关闭  
	//如果有用户指针 还是无法保证 最后一个指针析构之前 会调用connectionDestroy  
	//如果最后一个指针在另外线程 那么无法在Conn析构函数保证处理loop线程的channel删除事件  
	//只能靠用户自己保证 在释放最后一个指针之前调用forceClose 将指针传递给loop线程
    	CloseCallback cb = std::bind(&detail::removeConnection, loop_, _1);
    	loop_->runInLoop(std::bind(&TcpConnection::setCloseCallback, conn, cb));
	if (unique)//如果其他地方没有连接就关闭 因为可以把连接暴露给外部 
    	{
	    //printf("close\n");
     	    conn->forceClose();
    	}
     }
     else
     {
    	connector_->stop();
     }

}

void TcpClient::connect()
{
    //如果当前已经连接了   多次调用有害
    LOG<< "TcpClient::connect[" << name_ << "] - connecting to " << connector_->serverAddress().toIpPort();
    connect_ = true;
    connector_->start();
}

void TcpClient::disconnect()
{
    connect_ = false;

    {
        MutexLockGuard lock(mutex_);
        if (connection_)
        {
            connection_->shutdown();
        }
    }
}

void TcpClient::stop()
{
    connect_ = false;
    connector_->stop();
}



void TcpClient::newConnection(int sockfd)
{
    LOG<<"Client new Conn";
    loop_->assertInLoopThread();
    InetAddress peerAddr(mysocket::getPeerAddr(sockfd));
    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
    ++nextConnId_;
    string connName = name_ + buf;

    InetAddress localAddr(mysocket::getLocalAddr(sockfd));
    TcpConnectionPtr conn(new TcpConnection(loop_,connName,sockfd,localAddr,peerAddr));

    if(connectionCallback_) //因为没有设置callback 导致连接建立访问空回调
	conn->setConnectionCallback(connectionCallback_);
    if(messageCallback_)conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpClient::removeConnection, this, _1));
    {
        MutexLockGuard lock(mutex_);
        connection_ = conn;
    }
    conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
    //printf("%p \n",loop_);
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());

    {
        MutexLockGuard lock(mutex_);
        assert(connection_ == conn);
        connection_.reset();
    }

    loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));

}
