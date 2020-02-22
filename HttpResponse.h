#pragma once

#include<string>
#include<map>

using std::string;

class Buffer;

//构造响应 把响应写入buffer
class HttpResponse
{
public:
    enum StatusCode
    {
        kUnknown,
        k200Ok = 200,
        k301MovedPermanently = 301,
        k400BadRequest = 400,
        k404NotFound = 404,
    };

    explicit HttpResponse(bool close)
        :status_(kUnknown),
         closeConn_(close)
        {

        }

    void appendToBuffer(Buffer* output) const;

    void setStatusString(const string& status){ statusString_ = status; }
    void setStatusCode(StatusCode status){ status_ = status; }

    void setClose(bool on){ closeConn_ = on;}
    bool closeConn()const {return closeConn_;}

    void addHeader(const string& key,const string value)
    {
        headers_[key] = value;
    }

    void setBody(const string& body){ body_ = body; }


private:
    std::map<string,string> headers_;
    StatusCode status_;
    string statusString_;
    bool closeConn_;
    string body_;

};
