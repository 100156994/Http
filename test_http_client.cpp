



#include "Logging.h"
#include "EventLoop.h"
#include "TcpClient.h"
#include "EventLoopThread.h"
#include "Thread.h"
#include"TcpConnection.h"


int main(int argc, char* argv[])
{
    EventLoopThread loopThread;
    {
        InetAddress serverAddr("127.0.0.1", 8000); // should succeed
        TcpClient client_3(loopThread.startLoop(), serverAddr, "TcpClient_3");//在另外的线程析构

        client_3.connect();
        sleep(1);
        int n=5;
        while(n--)
        {
            TcpConnectionPtr conn = client_3.connection();
            conn->send("GET /hello HTTP/1.1\r\nConnection: Keep-Alive\r\n\r\n");
            sleep(1);
        }

        sleep(1);
        client_3.disconnect();
	sleep(1);
    }
}
