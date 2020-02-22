#include"Channel.h"
#include <sys/epoll.h>
#include "EventLoop.h"

#include <sstream>
#include"Logging.h"

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop* loop, int fd)
    :loop_(loop),
     fd_(fd),
     events_(0),
     revents_(0),
     index_(-1),
     eventHandling_(false),
     addedToLoop_(false)
{

}

Channel::~Channel()
{
    //connector的channel写事件时 会reset channel 所以把reset放到pendingfunc中执行  然后把fd暴露给客户 这是正在处理channel 所以channel析构 这样导致handle时 自身析构
    //printf("~ %d\n",fd_);
    assert(!eventHandling_);
    //客户析构时候connector 的channel析构时 
    //assert(!addedToLoop_);
    if (loop_->isLoopInThread())
    {
    	//assert(!loop_->hasChannel(this));
    }
}

void Channel::handleEvent(size_t receiveTime)
{
    eventHandling_ = true;
    //printf("handle Event %d\n",fd_);
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        //对方挂断且无数据可读
        events_ = 0;
        //log
        //printf("close callback\n");
        //LOG<<"HUP "<<revents_;
        
	if(closeCallback_) closeCallback_();
        //return;
    }
    if (revents_ & EPOLLERR)
    {
        if (errorCallback_)
            errorCallback_;
        events_ = 0;
        //return;
    }
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
    {
        //printf("read %d\n",this->fd());
        if(readCallback_)
            readCallback_(receiveTime);
    }
    if (revents_ & EPOLLOUT)
    {
	//printf("write \n");
        if(writeCallback_)
            writeCallback_();
    }
    eventHandling_ = false;
    //printf("handle over\n");
}
void Channel::update()
{
    addedToLoop_ = true;
    loop_->updateChannel(this);
}

void Channel::remove()
{
    assert(isNoneEvent());
    addedToLoop_ = false;
    loop_->removeChannel(this);
}


string Channel::reventsToString() const
{
    return eventsToString(fd_, revents_);
}

string Channel::eventsToString() const
{
    return eventsToString(fd_, events_);
}

string Channel::eventsToString(int fd, int ev)
{
    std::ostringstream oss;
    oss << fd << ": ";
    if (ev & EPOLLIN)
      oss << "IN ";
    if (ev & EPOLLPRI)
      oss << "PRI ";
    if (ev & EPOLLOUT)
     oss << "OUT ";
    if (ev & EPOLLHUP)
      oss << "HUP ";
    if (ev & EPOLLRDHUP)
      oss << "RDHUP ";
    if (ev & EPOLLERR)
      oss << "ERR ";
    if (ev & EPOLLET)
      oss << "ET ";
    if(ev & EPOLLONESHOT)
      oss << "ONESHOT ";

    return oss.str();
}
