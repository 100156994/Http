
#pragma once
#include<functional>
#include"noncopyable.h"

class EventLoop;

//负责一个fd的事件分发 但不负责fd的关闭
class Channel : noncopyable
{
public:
    typedef std::function<void()> EventCallback;
    typedef std::function<void(size_t)> ReadEventCallback;

    Channel(EventLoop* loop ,int fd);
    ~Channel();

    void handleEvent(size_t receiveTime);
    void setReadCallback(ReadEventCallback cb)
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
    void setCloseCallback(EventCallback cb)
    {
        closeCallback_ = std::move(cb);
    }
    int fd() const { return fd_; }
    int events() const { return events_; }
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
    void update();
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_;
    const int fd_;
    int event_;
    int revent_;
    int index_;//epoller中map的key

    bool eventHandling_;
    bool addedToLoop_;
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback errorCallback_;
    EventCallback closeCallback_;

    //防止 channel在hand时 conn析构导致回调conn的函数出现未知错误
    ///个人感觉 connptr 会在server的removeConn中持有服务器的最后一个（如果客户也没有） 然后将bind connDestroy
    ///然后queue进loop线程 但排队回调会在全部handleEvent调用结束后 才会调用doPengdingFunc  所以connDestroy 会在doPengdingFunc 时结束
    ///这样conn 的析构不可能早于handleEvent调用结束 并不需要tie
    //std::weak_ptr<void> tie_;
    //bool tied_;
};
