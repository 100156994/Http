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
    //connector��channelд�¼�ʱ ��reset channel ���԰�reset�ŵ�pendingfunc��ִ��  Ȼ���fd��¶���ͻ� �������ڴ���channel ����channel���� ��������handleʱ ��������
    //printf("~ %d\n",fd_);
    assert(!eventHandling_);
    //�ͻ�����ʱ��connector ��channel����ʱ 
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
        //�Է��Ҷ��������ݿɶ�
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
