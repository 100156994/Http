#include"Channel.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>

const int EVENTSNUM = 16;//最初默认一次接收的事件最大值 到阈值翻倍
const int kNew = -1;  //当前不在poll map中 没有被监听
const int kAdded = 1; //在poll map中 监听
const int kDeleted = 2; //在pool map，没有监听

Epoller::Epoller(EventLoop* loop)
    :loop_(loop),
     epollfd_(epoll_create1(EPOLL_CLOEXEC)),
     events_(EVENTSNUM)
    {
        assert(epollfd_>0);
    }

Epoller::~Epoller()
{
    close(epollfd_);
}


void Epoller::poll(int timeoutMs,ChannelList* activeChannels)
{
    assertInLoopThread();
    struct epoll_event event;
    int eventsNum=epoll_wait(epollfd_,&*events_.begin(),static_cast<int>(events_.size()),timeoutMs);
    if(eventsNum>0)//成功
    {
        fillActiveChannels(eventsNum,activeChannels);
        if(static_cast<int>(events_.size()) = eventsNum )
        {
            events_resize(events_.size()*2);
        }
    }else if(eventsNum==0)//超时
    {
        //log
    }else//出错
    {
        //log
    }

}


void Epoller::updateChannel(Channel *channel)
{
    assertInLoopThread();
    const int index = channel->index();
    if(index ==kNew || index == kDeleted)//add
    {
        int fd = channel->fd();
        if (index == kNew)
        {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        }else // index == kDeleted
        {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }
        channel->set_index(kAdded);
        update(EPOLL_CLT_ADD,channel);

    }else//修改
    {
        int fd =channel->fd();

        assert(channels_.find(channel)!=Channels_.end());
        assert(Channels_[fd]==channel);
        assert(index==kAdded);
        if(channel.isNoneEvent())
        {
            update(EPOLL_CTL_DEL,channel);
            channel.set_index(kDeleted);
        }else
        {
            update(EPOLL_CTL_MOD,channel);
        }

    }
}
void Epoller::removeChannel(Channel* channel)
{
    assertInLoopThread();
    int fd = channel.fd();
    int index= channel.index();
    assert(channels_.find(channel)!=Channels_.end());
    assert(Channels_[fd]==channel);
    assert(index==kAdded||index==kDeleted);

    size_t n = Channels_.erase(fd);
    assert(n==1);

    if(index==kAdded)
    {
        update(EPOLL_CTL_DEL,channel);
    }
    channel.set_index(kNew);
}

bool EventLoop::hasChannel(Channel* channel)
{
    assertInLoopThread();
    ChannelMap::const_iterator it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}


void Epoller::fillActiveChannels(int eventsNum,ChannelList* activeChannels)const
{
    assert(static_cast<size_t>(numEvents) <= events_.size());
    for(int i = 0; i < eventsNum; ++i)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
#ifndef NDEBUG
        int fd = channel->fd();
        ChannelMap::const_iterator it = channels_.find(fd);
        assert(it != channels_.end());
        assert(it->second == channel);
#endif
        Channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}


void Epoller::update(int operation, Channel* channel)
{
    struct epoll_event event;
    event.events=channel->revent_;
    event.data.ptr=channel;
    const int fd =channel->fd();
    if(epoll_ctl(epollfd_,operation,fd,&event)<0)//error
    {
        //log
    }
}
