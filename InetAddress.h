#pragma once

#include<netinet/in.h>
#include <sys/socket.h>
#include<string>

struct sockaddr_in;
using std::string;

class InetAddress
{

public:
    explicit InetAddress(uint16_t port,bool loopbackOnly = false);

    InetAddress(string ip,uint16_t port);

    explicit InetAddress(const struct sockadd_in& addr)
        :addr_(addr);
    //ʹ��Ĭ�Ͽ���


    sa_family_t family()const
    {
        return addr_.sin_family;
    }
    string toIp()const;
    string toIpPort()const;
    uint16_t toPort()const;

    const struct sockaddr* getSockAddr() const { return static_cast<const struct sockaddr*>(&addr_); }

    //�����ֽ���
    uint32_t ipNetEndian() const;
    uint16_t portNetEndian() const{ return addr_.sin_port; }

    //��������ȡ������ַ
    static bool resolve(StringArg hostname, InetAddress* result);
private:
    struct sockaddr_in addr_;


};
