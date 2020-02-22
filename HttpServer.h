#pragma once

#include"noncopyable.h"
#include"TcpServer.h"
#include"HttpRequest.h"
#include"HttpResponse.h"
#include<functional>

//基于TcpServe 构建HttpServer   
class HttpServer : noncopyable
{
public:
    typedef std::function<void(const HttpRequest& ,HttpResponse *)> HttpCallBack;

    HttpServer(EventLoop* loop,const InetAddress& listenAddr,const string serverName,TcpServer::Option option = TcpServer::kNoReusePort);
    ~HttpServer(){};

    void setCallBack(const HttpCallBack& cb){ callback_ = cb; }

    void setThreadNums(int n){ tcpServer_.setThreadNum(n); }

    void start();


private:
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn,Buffer* buf,size_t receiveTime);//会一直读请求 直到buffer读取完毕
    void onRequest(const TcpConnectionPtr&, const HttpRequest&);


    TcpServer tcpServer_;
    HttpCallBack callback_;

};
