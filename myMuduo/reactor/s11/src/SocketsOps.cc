#include "../lib/SocketsOps.h"
#include "../lib/muduo/base/Logging.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h> //snprintf
#include <strings.h> //bzero
#include <sys/socket.h>
#include <unistd.h> //封装了类UNIX系统下的很多固定名称的system_call系统调用。
                    //所以，这个函数是依赖于编译器，依赖于操作系统的

using namespace muduo;

namespace{

typedef struct sockaddr SA;

const SA* sockaddr_cast(const struct sockaddr_in* addr){
    return static_cast<const SA*>(implicit_cast<const void*>(addr));
}

SA* sockaddr_cast(struct sockaddr_in* addr){
    return static_cast<SA*>(implicit_cast<void*>(addr));
}

void setNonBlockAndCloseOnExec(int sockfd){
    //non-block
    int flags = ::fcntl(sockfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    int ret = ::fcntl(sockfd, F_SETFL, flags);
    //FIXME check

    //close-on-exec
    flags = ::fcntl(sockfd, F_GETFD, 0);
    flags |= FD_CLOEXEC;
    ret = ::fcntl(sockfd, F_SETFD, flags);
    //FIXME check
}

}

int sockets::createNonblockingOrDie(){
    //socket
#if VALGRIND
    int sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd < 0){
        LOG_SYSFATAL << "sockets::createNonblockingOrDie";
    }

    setNonBlockAndCloseOnExec(sockfd);
#else
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if(sockfd < 0){
        LOG_SYSFATAL << "sockets::createNonblockingOrDie";
    }
#endif
    return sockfd;
}

int sockets::connect(int sockfd, const struct sockaddr_in& addr){
    return ::connect(sockfd, sockaddr_cast(&addr), sizeof addr);
}

void sockets::bindOrDie(int sockfd, const struct sockaddr_in& addr){
    int ret = ::bind(sockfd, sockaddr_cast(&addr), sizeof addr);
    if(ret < 0){
        LOG_SYSFATAL << "sockets::bindOrDie";
    }
}

void sockets::listenOrDie(int sockfd){
    int ret = ::listen(sockfd, SOMAXCONN);
    if(ret < 0){
        LOG_SYSFATAL << "sockets::listenOrDie";
    }
}

int sockets::accept(int sockfd, struct sockaddr_in* addr){
    socklen_t addrlen = sizeof *addr;
#if VALGRIND
    int connfd = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
    setNonBlockAndCloseOnExec(connfd);
#else  
    int connfd = ::accept4(sockfd, sockaddr_cast(addr), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    //sockfd 包含了服务器的端口和ip
    //Sockfd contains the server port and IP address
    //addr 客户端的地址
    //Addr Indicates the address of the client
    //返回的connfd 包含了客户端的端口和ip
    //The connfd returned contains the port and IP of the client
#endif

    if(connfd < 0){
        int savedErrno = errno;
        LOG_SYSERR << "Socket::accept";
        switch(savedErrno){
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO:
            case EPERM:
            case EMFILE: //per-process lmit of open file desctiptor
                //expected errors
                errno = savedErrno;
                break;
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTSUPP:
                //unexpected errors
                LOG_FATAL << "unexpected error of ::accept " << savedErrno;
                break;
            default:
                LOG_FATAL << "unknown error of ::accept" << savedErrno;
                break;
        }
    }
    return connfd;
}

void sockets::close(int sockfd){
    if(::close(sockfd) < 0){
        LOG_SYSERR << "sockets::close";
    }
}

void sockets::shutdownWrite(int sockfd){
    if(::shutdown(sockfd, SHUT_WR) < 0){
        LOG_SYSERR << "sockets::shutdownWrite";
    }
}

void sockets::toHostPort(char* buf, size_t size, const struct sockaddr_in& addr){ //暂时理解为保存客户端的ip地址等
    char host[INET_ADDRSTRLEN] = "INVALID";
    ::inet_ntop(AF_INET, &addr.sin_addr, host, sizeof host);
    //p代表表达, n代表数值
    //将数值格式转化为点分十进制的ip地址格式
    //ip地址格式结果保存在host
    //p for expression, n for number  
    // Convert the value format to the IP address format in dotted decimal notation  
    // IP address format The result is saved to host  
    uint16_t port = sockets::networkToHost16(addr.sin_port);
    snprintf(buf, size, "%s:%u", host, port);
}

void sockets::fromHostPort(const char* ip, uint16_t port, struct sockaddr_in* addr){ //暂时理解为向目标端口发送本机信息
    addr->sin_family = AF_INET;
    addr->sin_port = hostToNetwork16(port);
    if(::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0){
        //将点分十进制的ip地址转化为用于网络传输的数值格式
        //数值格式结果保存在addr->sin_addr中
        // Convert an IP address in dotted decimal notation to a numeric format for network transmission  
        // The result is stored in addr->sin_addr  
        LOG_SYSERR << "sockets::fromHostPort";
    }
}

struct sockaddr_in sockets::getLocalAddr(int sockfd){
    struct sockaddr_in localaddr;
    bzero(&localaddr, sizeof localaddr);
    socklen_t addrlen = sizeof(localaddr);
    if(::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0){
        LOG_SYSERR << "sockets::getLocalAddr";
    }
    return localaddr;
}

struct sockaddr_in sockets::getPeerAddr(int sockfd){
    struct sockaddr_in peeraddr;
    bzero(&peeraddr, sizeof peeraddr);
    socklen_t addrlen = sizeof(peeraddr);
    if(::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0){
        LOG_SYSERR << "sockets::getPeerAddr";
    }
    return peeraddr;
}

int sockets::getSocketError(int sockfd){
    int optval;
    socklen_t optlen = sizeof optval;

    if(::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0){ //getsockopt()获取一个套接字选项
                                                                          //第二个参数是获取协议层次
                                                                          //第三个是获取错误状态
                                                                          //getsockopt() gets a socket option
                                                                          //The second argument is to get the protocol level
                                                                          //The third is to get the error status
        return errno;
    }else{
        return optval;
    }
}

bool sockets::isSelfConnect(int sockfd){
    struct sockaddr_in localaddr = getLocalAddr(sockfd);
    struct sockaddr_in peeraddr = getPeerAddr(sockfd);
    return localaddr.sin_port  == peeraddr.sin_port && 
           localaddr.sin_addr.s_addr == peeraddr.sin_addr.s_addr;
}