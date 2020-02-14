

#include"InetAddress.h"
#include<stdint.h>
#include<endian.h>
#include <sys/socket.h>
#include <netdb.h>
#include<stdio.h>

//     /* Structure describing an Internet socket address.  */
//     struct sockaddr_in {
//         sa_family_t    sin_family; /* address family: AF_INET */
//         uint16_t       sin_port;   /* port in network byte order */
//         struct in_addr sin_addr;   /* internet address */
//     };

//     /* Internet address. */
//     typedef uint32_t in_addr_t;
//     struct in_addr {
//         in_addr_t       s_addr;     /* address in network byte order */
//     };

namespace socket
{
void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr)
{
    addr->family = AF_INET:
    addr->sin_port = htobe16(port);
    if(inet_pton(AF_INET,ip,&addr->sin_addr)<=0)
    {//log
        printf("fromIpport error\n");
    }
};

}


using namespace socket;

InetAddress::InetAddress(uint16_t port,bool loopbackOnly = false)
{
    memset(&addr_,0,sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htobe16(port);
    in_addr_t ip= loopbackOnly?INADDR_LOOPBACK:INADDR_ANY;
    addr_.sin_addr.s_addr = htobe32(ip);


}

InetAddress::InetAddress(string ip,uint16_t port)
{
    memset(&addr_,0,sizeof(addr_));
    fromIpPort(ip.c_str(),port,&addr_);
}


string InetAddress::toIp()
{
    char buf[64] = "";
    assert(64==INET_ADDRSTRLEN);
    inet_ntop(AF_INET,&addr_->sin_addr,buf,INET_ADDRSTRLEN);
    return buf;
}

string InetAddress::toIpPort()
{
    string ret;
    ret = toIp();
    size_t len = ::strlen(buf);
  const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
    uint16_t port = toPort();
    char buf[64] = "";
    snprintf(buf, 64, ":%u", port);
    ret+=buf;
    return ret;
}

uint16_t InetAddress::toPort()
{
    return betoh16(portNetEndian())
}

static __thread char t_resolveBuffer[8192];//Ïß³Ì»º³å

bool InetAddress::resolve(string hostname, InetAddress* result)
{
    assert(result!=nullptr);
    struct hostent hent;
    memset(&hent,0,sizeof(hent));
    struct hostent* he=nullptr;
    int herrno =0;
    int ret = gethostbyname_r(hostname.c_str(),&hent,t_resolveBuffer,sizeof(t_resolveBuffer),&he,&herrno);
    if(ret ==0 && he !=NULL)
    {
        assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
        assert(he == &hent);
        result->addr_.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
        return true;
    }else{
        if(ret)
        {//log
            printf("InetAddress::resolve\n");
        }
        return false;
    }
}
