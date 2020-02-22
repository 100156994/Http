#include"HttpResponse.h"

#include"Buffer.h"
#include<stdio.h>

//版本 状态码 状态字符串/r/n
//首部选项： 首部内容/r/n
//....
///r/n
//body

void HttpResponse::appendToBuffer(Buffer* output)const
{
    char buf[32];
    snprintf(buf,sizeof(buf),"HTTP/1.1 %d ",status_);
    output->append(buf);
    output->append(statusString_);
    output->append("\r\n");

    if(closeConn_)
    {
        output->append("Connection: close\r\n");
    }else
    {
        snprintf(buf, sizeof(buf), "Content-Length: %zd\r\n", body_.size());
        output->append(buf);
        output->append("Connection: Keep-Alive\r\n");
    }

    for (const auto& header : headers_)
    {
        output->append(header.first);
        output->append(": ");
        output->append(header.second);
        output->append("\r\n");
    }

    output->append("\r\n");
    output->append(body_);
}
