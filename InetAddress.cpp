

#include"InetAddress.h"
#

InetAddress::InetAddress(uint16_t port,bool loopbackOnly = false)
{
    memset(&addr_,0,sizeof(addr_))
    addr_.sin_family = AF_INET;
    addr_.sin_port = port;
    in_addr_t = loopbackOnly?INADDR_LOOPBACK:INADDR_ANY;
    addr_.sin_addr.s_addr=


}

InetAddress::InetAddress(string ip,uint16_t port);
