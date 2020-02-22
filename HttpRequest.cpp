
#include"HttpRequest.h"
#include<algorithm>

//解析请求  目前之解析到 请求行 和首部  
bool HttpContext::parse(Buffer* buf,size_t receiveTime)
{
    bool running = true;
    while(running)
    {
        if(state_ == kExpectRequestLine)
        {
            const char* crlf =buf->findCRLF();
            if(crlf)
            {
                if(parseLine(buf->peek(),crlf))
                {
                    buf->retrieveUntil(crlf+2);
                    state_ = kExpectHeaders;
                }else
                {
                    buf->retrieveUntil(crlf+2);
                    return false;
                }
            }else
            {
                buf->retrieveAll();
                return false;
            }
        }else if(state_ == kExpectHeaders)
        {
            const char* crlf =buf->findCRLF();
            if(crlf)
            {
                if(buf->peek() == crlf)//读到空行 头部结束
                {
                    state_ = kGotAll;
                    buf->retrieveUntil(crlf+2);
                    request_.setReceiveTime(receiveTime);
                    return true;
                }else
                {
                    if(parseHead(buf->peek(),crlf))
                    {
                        buf->retrieveUntil(crlf+2);
                        state_ = kExpectHeaders;
                    }else
                    {
                        buf->retrieveUntil(crlf+2);
                        return false;
                    }
                }
            }else
            {
                buf->retrieveAll();
                return false;
            }
        }else if(state_ == kExpectBody)
        {
            ///to-do  如果支持post方法
        }
    }
    return false;

}



//HTTP请求行
bool HttpContext::parseLine(const char* begin,const char* end)
{
    const char* start = begin;
    const char* space = std::find(start, end, ' ');
    if(space !=end && request_.setMethod(start,space))
    {
        start = space+1;
        space = std::find(start,end,' ');
        if(space != end)
        {
            const char* query = std::find(start,space,'?');
            if(query != space)
            {
                request_.setPath(start,query);
                request_.setQuery(query,space);
            }else{
                request_.setPath(start,space);
            }
            start = space+1;
            if(std::equal(start,end,"HTTP/1.1")||std::equal(start,end,"HTTP/1.0"))
                return true;
        }
    }
    return false;
}
//HTTP请求首部行
bool HttpContext::parseHead(const char* begin,const char* end)
{
    const char* colon = std::find(begin, end, ':');
    if (colon != end)
    {
        request_.addHeader(begin, colon, end);
        return true;
    }
    else
    {
        return false;
    }
}
