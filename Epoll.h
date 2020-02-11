
#pragma once

#include<map>
#include<vector>
#include"noncopyable.h"
#include"EventLoop.h"
#include<vector>
#include<map>
struct epoll_event;
class Channel;
class EventLoop;

//IO复用  不拥有Channel
class Epoller : noncopyable
{
public:
    typedef std::vector<Channel*> ChannelList;
    Epoller(EventLoop* loop);
    ~Epoller();

    //只能在loop线程调用
    void poll(int timeoutMs,ChannelList* activeChannels);
    void updateChannel(Channel *);//删除只是不在监听 在map中保留
    void removeChannel(Channel *);//从map中删除
    bool hasChannel(Channel*);

    //
    void assertInLoopThread();

private:
    void fillActiveChannels(int ,ChannelList* )const;
    void update(int operation,Channel* channel);
    typedef std::map<int,Channel*> ChannelMap;
    typedef std::vector<struct epoll_event> EventList;
    EventLoop* loop_;
    int epollfd_;
    EventList events_;
    ChannelMap channels_;

};
