
#pragma once
#include<functional>
#include"noncopyable.h"

class EventLoop;

//����һ��fd���¼��ַ� ��������fd�Ĺر�
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
    int index(){ return index_; }//����channel��״̬ -1 1 2
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
    int index_;//epoller��map��key

    bool eventHandling_;
    bool addedToLoop_;
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback errorCallback_;
    EventCallback closeCallback_;

    //��ֹ channel��handʱ conn�������»ص�conn�ĺ�������δ֪����
    ///���˸о� connptr ����server��removeConn�г��з����������һ��������ͻ�Ҳû�У� Ȼ��bind connDestroy
    ///Ȼ��queue��loop�߳� ���Ŷӻص�����ȫ��handleEvent���ý����� �Ż����doPengdingFunc  ����connDestroy ����doPengdingFunc ʱ����
    ///����conn ����������������handleEvent���ý��� ������Ҫtie
    //std::weak_ptr<void> tie_;
    //bool tied_;
};
