#include "Logging.h"
#include "EventLoop.h"
#include "TcpClient.h"
#include "EventLoopThread.h"
#include "Thread.h"
#include<stdio.h>


TcpClient* g_client;

void timeout()
{
    printf("time out\n");
    LOG<< "timeout";
    g_client->stop();
}

void threadFunc(EventLoop* loop)
{
    InetAddress serverAddr("127.0.0.1", 8000); // should succeed
    TcpClient client(loop, serverAddr, "TcpClient_2"); // client destructs when connected.
    client.connect();

    sleep(1);
	client.disconnect();

}

int main(int argc, char* argv[])
{
    EventLoopThread loopThread;
    {
        InetAddress serverAddr("127.0.0.1", 8000); // should succeed
        TcpClient client_3(loopThread.startLoop(), serverAddr, "TcpClient_3");//在另外的线程析构
        client_3.connect();
        sleep(1);
        client_3.disconnect();
    }

    EventLoop loop;
    InetAddress serverAddr("127.0.0.1", 9945); // no such server
    TcpClient client(&loop, serverAddr, "TcpClient_1");//同时连接和关闭
    g_client = &client;
    loop.runAfter(1.0, timeout);
    
    client.connect();

    Thread thr(std::bind(threadFunc, &loop));
    thr.start();
    loop.runAfter(15.0, std::bind(&EventLoop::quit, &loop));
    loop.loop();
}
