
#include"HttpServer.h"
#include"Logging.h"
#include"HttpRequest.h"

namespace detail
{
void defaultHttpCallback(const HttpRequest&, HttpResponse* resp)
{
	//printf("defalut \n");
    resp->setStatusCode(HttpResponse::k404NotFound);
    resp->setStatusString("Not Found");
    resp->setClose(true);
}

}

using detail::defaultHttpCallback;

HttpServer::HttpServer(EventLoop* loop,const InetAddress& listenAddr,const string serverName,TcpServer::Option option)
    :tcpServer_(loop,listenAddr,serverName,option),
     callback_(&defaultHttpCallback)
    {
        tcpServer_.setConnectionCallback(std::bind(&HttpServer::onConnection,this,_1));
        tcpServer_.setMessageCallback(std::bind(&HttpServer::onMessage,this,_1,_2,_3));
    }


void HttpServer::start()
{
    //LOG<< "HttpServer[" << tcpServer_.name()<< "] starts listenning on " << tcpServer_.ipPort();
    tcpServer_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr& conn)
{
    if (conn->connected())
    {
        //LOG<< "HttpServer[" << tcpServer_.name()<< "]  connection connected " ;
    }else{
        //LOG<< "HttpServer[" << tcpServer_.name()<< "]  connection diconnected " ;
    }
}

void HttpServer::onMessage(const TcpConnectionPtr& conn,Buffer* buf,size_t receiveTime)
{
    ///循环读直到buffer读完
    HttpContext context;

	
	
    while(buf->readableBytes()>0)
    {
	//string re = buf->retrieveAllAsString();
        
	//printf("on message  %s\n",re.c_str());
        if (!context.parse(buf, receiveTime))
        {
            conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
            conn->shutdown();
        }
        if (context.gotAll())
        {
	    //printf("on request  \n");
            onRequest(conn, context.request());
        }
    }



}

void HttpServer::onRequest(const TcpConnectionPtr& conn, const HttpRequest& req)
{
    const string& connection = req.getHeader("Connection");
    //printf("%s\n",connection.c_str());
    //bool close = true;
    bool close = connection == "close";
    HttpResponse response(close);
    callback_(req, &response);
    Buffer buf;
    response.appendToBuffer(&buf);

    //string send = buf.retrieveAllAsString();
    //printf("close  %d send = %s\n",response.closeConn(),send.c_str());  
    //conn->send(send);
    conn->send(&buf);
    if (response.closeConn())
    {
        conn->shutdown();
    }
}
