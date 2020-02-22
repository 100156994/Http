
#pragma once
#include"Epoll.h"
#include"CurrentThread.h"
#include"Channel.h"
#include"TimerQueue.h"
#include<functional>
#include"MutexLock.h"
#include<memory>
#include<string>
#include <unistd.h>
#include"Logging.h"
class Epoller;
class Channel;
class TimerQueue;

using  std::string;


//һ���߳�ֻ����һ��loop  ���̵߳���ʹ��������  ����������epoll���лʱ���  �Ż�ִ�п��߳�����  ����ݹ���ӣ������첽������һ��ִ��
//��һ����ʱ������ ���첽����channel fd(��loop������ر�)
class EventLoop
{
public:
    typedef std::function<void()> Functor;
    typedef std::function<void()> TimerCallback;
    EventLoop();
    ~EventLoop();

    //ֻ���ڴ����̵߳���
    void loop();
    void runInLoop(Functor cb);
    void queueInLoop(Functor cb);
    size_t queueSize()const ;

    void quit();//���̵߳�����һ������

    //��ʱ������
    TimerId runAt(size_t time, TimerCallback cb);
    TimerId runAfter(double delay, TimerCallback cb);
    TimerId runEvery(double interval, TimerCallback cb);
    void cancel(TimerId timerId);

    //internal  �����߳��첽���� ��chanel-remove
    void wakeup();
    void updateChannel(Channel*);
    void removeChannel(Channel*);
    bool hasChannel(Channel*);




    bool eventHanding()const {return eventHanding_;}


    bool isLoopInThread()const { return threadId_ == CurrentThread::tid();}
    void assertInLoopThread()
    {
        if(!isLoopInThread())
        {
            abortNotInLoopThread();
        }
    }
    static EventLoop* getEventLoopOfCurrentThread();
private:
    void abortNotInLoopThread();
    void doPendingFunctors();
    void handleRead();



    typedef std::vector<Channel*> ChannelList;

    bool looping_;
    bool quit_;
    const pid_t threadId_;
    bool eventHanding_;
    bool callPendingFunctors_;
    std::unique_ptr<Epoller> poller_;
    std::unique_ptr<TimerQueue> timerQueue_;

    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;


    ChannelList activeChannels_;
    Channel* currentActiveChannel_;

    mutable MutexLock mutex_;
    std::vector<Functor> pendingFunctors_;
};
