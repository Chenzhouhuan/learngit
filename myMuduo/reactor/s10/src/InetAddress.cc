#include "../lib/InetAddress.h"
#include "../lib/SocketsOps.h"

#include <strings.h> //bzero
#include <netinet/in.h>

#include <boost/static_assert.hpp>

using namespace muduo;

static const in_addr_t kInaddrAny = INADDR_ANY;

BOOST_STATIC_ASSERT(sizeof(InetAddress) == sizeof(struct sockaddr_in)); //在编译期间就需要确认某些表达式是否是我们所期望的那样
                                                                        //At compile time we need to verify that certain expressions are as expected

InetAddress::InetAddress(uint16_t port){
    bzero(&addr_, sizeof addr_);
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = sockets::hostToNetwork32(kInaddrAny);
    addr_.sin_port = sockets::hostToNetwork16(port);
}

InetAddress::InetAddress(const std::string& ip, uint16_t port){
    bzero(&addr_, sizeof addr_);
    sockets::fromHostPort(ip.c_str(), port, &addr_);
}

std::string InetAddress::toHostPort() const{
    char buf[32];
    sockets::toHostPort(buf, sizeof buf, addr_);
    return buf;
}