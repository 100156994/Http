#include"EventLoop.h"
#include"Thread.h"

EventLoop* g_loop;

void threadFunc()
{
    g_loop->loop();
}


int main()
{
    //test 1 ���ڱ��̵߳���loop
    EventLoop loop;
    g_loop = &loop;
    Thread t1(threadFunc);
    t1.start();
    t1.join();

}
