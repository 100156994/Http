
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

//IO����  ��ӵ��Channel
class Epoller : noncopyable
{
public:
    typedef std::vector<Channel*> ChannelList;
    Epoller(EventLoop* loop);
    ~Epoller();

    //ֻ����loop�̵߳���
    void poll(int timeoutMs,ChannelList* activeChannels);
    void updateChannel(Channel *);//ɾ��ֻ�ǲ��ڼ��� ��map�б���
    void removeChannel(Channel *);//��map��ɾ��
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
