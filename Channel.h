
#pragma once
#include<functional>
#include"noncopyable.h"
#include<string>
class EventLoop;


using std::string;
//负责一个fd的事件分发 但不负责fd的关闭
class Channel : noncopyable
{
public:
    typedef std::function<void()> EventCallback;

    Channel(EventLoop* loop ,int fd);
    ~Channel();

    void handleEvent();
    void setReadCallback(EventCallback cb)
    {
        readCallback_ = std::move(cb);
    }
    void setWriteCallback(EventCallback cb)
    {
        writeCallback_ = std::move(cb);
    }
    void setErrorCallback(EventCallback cb)
    {
        errorCallback_ = std::move(cb);
    }
    int fd() const { return fd_; }
    int events() const { return events_; }
    int revents() const { return revents_; }
    void set_revents(int revt) { revents_ = revt; } // used by pollers

    bool isNoneEvent() const { return events_ == kNoneEvent; }
    void enableReading() { events_ |= kReadEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }
    // for Poller
    int index(){ return index_; }//代表channel的状态 -1 1 2
    void set_index(int idx){ index_ = idx; }
    //for debug
    string reventsToString() const;
    string eventsToString() const;
    EventLoop* ownerLoop()
    {
        return loop_;
    }
    void remove();
private:
    static string eventsToString(int fd,int ev);

    void update();
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_;
    const int fd_;
    int events_;
    int revents_;
    int index_;//epoller中map的key

    bool eventHandling_;
    bool addedToLoop_;
    EventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback errorCallback_;
};
