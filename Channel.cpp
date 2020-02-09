#include"Channel.h"
#include <sys/epoll.h>
#include "EventLoop.h"

#include <sstream>


const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd)
    :loop_(loop_),
     fd(fd),
     event_(0),
     revent_(0),
     index_(-1),
     eventHandling_(false),
     addedToLoop_(false)
{

}

Channel::~Channel()
{
  assert(!eventHandling_);
  assert(!addedToLoop_);
  if (loop_->isInLoopThread())
  {
    assert(!loop_->hasChannel(this));
  }
}

Channel::handleEvent()
{
    eventHandling_ = true;
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        //对方挂断且无数据可读
        events_ = 0;

        return;
    }
    if (revents_ & EPOLLERR)
    {
        if (errorHandler_)
            errorHandler_();
        events_ = 0;
        return;
    }
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
    {
        if(readCallback_)
            readCallback_();
    }
    if (revents_ & EPOLLOUT)
    {
        if(writeCallback_)
            writeCallback_();
    }
    eventHandling_ = false;
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
  if (ev & POLLIN)
    oss << "IN ";
  if (ev & POLLPRI)
    oss << "PRI ";
  if (ev & POLLOUT)
    oss << "OUT ";
  if (ev & POLLHUP)
    oss << "HUP ";
  if (ev & POLLRDHUP)
    oss << "RDHUP ";
  if (ev & POLLERR)
    oss << "ERR ";
  if (ev & POLLNVAL)
    oss << "NVAL ";

  return oss.str();
}
