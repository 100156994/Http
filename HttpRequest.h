#pragma once

#include"Buffer.h"
#include<string>
#include<map>
#include<assert.h>


using std::string;

class HttpRequest
{
public:
    enum Method
    {
        kInvalid, kGet, kPost, kHead, kPut, kDelete
    };

    HttpRequest():method_(kInvalid){}
    ~HttpRequest()=default;

    bool setMethod(const char* start, const char* end)
    {
        assert(method_ == kInvalid);
        string m(start, end);
        if (m == "GET"){
            method_ = kGet;
        }else if (m == "POST"){
            method_ = kPost;
        }else if (m == "HEAD")
        {
            method_ = kHead;
        }else if (m == "PUT"){
            method_ = kPut;
        }else if (m == "DELETE"){
            method_ = kDelete;
        }else{
            method_ = kInvalid;
        }
        return method_ != kInvalid;
    }

    Method method() const  { return method_; }

    const char* methodString() const
    {
        const char* result = "UNKNOWN";
        switch(method_)
        {
            case kGet:
                result = "GET";
                break;
            case kPost:
                result = "POST";
                break;
            case kHead:
                result = "HEAD";
                break;
            case kPut:
                result = "PUT";
                break;
            case kDelete:
                result = "DELETE";
                break;
            default:
                break;
        }
        return result;
    }

    void setPath(const char* start, const char* end){ path_.assign(start, end); }

    const string& path() const{ return path_; }

    void setQuery(const char* start, const char* end){ query_.assign(start, end);}

    const string& query() const{ return query_; }

    void setReceiveTime(size_t t){ receiveTime_ = t; }

    size_t receiveTime() const{ return receiveTime_; }

    void addHeader(const char* start, const char* colon, const char* end)
    {
        //获取一行的string
        string field(start, colon);
        ++colon;
        while (colon < end && isspace(*colon))
        {
            ++colon;
        }
        string value(colon, end);
        while (!value.empty() && isspace(value[value.size()-1]))
        {
            value.resize(value.size()-1);
        }
        headers_[field] = value;
    }

    string getHeader(const string& field) const
    {
        string result;
        std::map<string, string>::const_iterator it = headers_.find(field);
        if (it != headers_.end())
        {
            result = it->second;
        }
        return result;
    }

    const std::map<string, string>& headers() const{ return headers_; }

    void swap(HttpRequest& that)
    {
        std::swap(method_, that.method_);
        path_.swap(that.path_);
        query_.swap(that.query_);
        std::swap(receiveTime_,that.receiveTime_);
        headers_.swap(that.headers_);
    }

private:
    Method method_;
    string path_;
    string query_;
    size_t receiveTime_;//微秒
    std::map<string, string> headers_;

};


class HttpContext
{
public:
    enum ParseState
    {
        kExpectRequestLine,
        kExpectHeaders,
        kExpectBody,
        kGotAll,
    };
    HttpContext():request_(),state_(kExpectRequestLine){}
    ~HttpContext()=default;

    bool parse(Buffer* buf,size_t receiveTime);


    bool gotAll() const{ return state_ == kGotAll; }

    const HttpRequest& request() const{ return request_; }

    HttpRequest& request(){ return request_; }
private:
    bool parseLine(const char* b,const char* e);
    bool parseHead(const char* b,const char* );
    HttpRequest request_;
    ParseState state_;
};
