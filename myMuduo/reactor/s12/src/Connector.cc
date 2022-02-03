#include "../lib/Connector.h"

#include "../lib/Channel.h"
#include "../lib/EventLoop.h"
#include "../lib/SocketsOps.h"

#include "../lib/muduo/base/Logging.h"

#include <boost/bind.hpp>

#include <errno.h>

using namespace muduo;

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
    : loop_(loop),
      serverAddr_(serverAddr),
      connect_(false),
      state_(kDisconnected),
      retryDelayMs_(kInitRetryDelayMs)
{
    LOG_DEBUG << "ctor[" << this << "]";
}

Connector::~Connector(){
    LOG_DEBUG << "dtor[" << this << "]";
    loop_->cancel(timerId_);
    assert(!channel_);
}

void Connector::start(){
    connect_ = true;
    loop_->runInLoop(boost::bind(&Connector::startInLoop, this)); // FIXME: unsafe
}

void Connector::startInLoop(){
    loop_->assertInLoopThread();
    assert(state_ == kDisconnected);
    if(connect_){
        connect();
    }else{
        LOG_DEBUG << "do not connect";
    }
}

void Connector::connect(){
    int sockfd = sockets::createNonblockingOrDie();
    int ret = sockets::connect(sockfd, serverAddr_.getSockAddrInet());
    int savedErrno = (ret == 0) ? 0 : errno;
    switch(savedErrno){
        case 0:
        case EINPROGRESS: //代表连接还在进行中
        case EINTR: //系统调用被中断
        case EISCONN: //表示连接已经建立
            connecting(sockfd);
            break;
        
        case EAGAIN: //再尝试一次(例如连续做read操作而没有数据可读，此时程序不会阻塞等待，而是放回该信号)
        case EADDRINUSE: //监听的端口已被使用
        case EADDRNOTAVAIL: //本地端口不够用
        case ECONNREFUSED: //被服务器拒绝
        case ENETUNREACH: //当前无法从本主机访问网络
            retry(sockfd);
            break;
        
        case EACCES: //没有权限操作
        case EPERM: //没有权限操作
        case EAFNOSUPPORT: //远程端点指定的地址族不能与这种类型的套接字一起使用
        case EALREADY: //超时，过时
        case EBADF: //无效描述符
        case EFAULT: //说明在对套接字上调用某些函数的时候传入了空指针等非法參数
        case ENOTSOCK: //表示sockfd是一个文件描述符而不是socket描述符
            LOG_SYSERR << "connect error in Connector::startInLoop " << savedErrno;
            sockets::close(sockfd);
            break;

        default:
            LOG_SYSERR << "Unexpected error in Connector::startInLoop " << savedErrno;
            sockets::close(sockfd);
            break;
    }
}

void Connector::restart(){
    loop_->assertInLoopThread();
    setState(kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

void Connector::stop(){
    connect_ = false;
    loop_->cancel(timerId_);
}

void Connector::connecting(int sockfd){
    setState(kConnecting);
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallback(boost::bind(&Connector::handleWrite, this)); // FIXME: unsafe
    channel_->setErrorCallback(boost::bind(&Connector::handleError, this)); // FIXME: unsafe

    channel_->enableWriting();
}

int Connector::removeAndResetChannel(){
    channel_->disableAll();
    loop_->removeChannel(get_pointer(channel_));
    int sockfd = channel_->fd();
    // Can't reset channel_ here, because we are inside Channel::handleEvent
    loop_->queueInLoop(boost::bind(&Connector::resetChannel, this)); // FIXME: unsafe
    return sockfd;
}

void Connector::resetChannel(){
    channel_.reset();
}

void Connector::handleWrite(){
    LOG_TRACE << "Connector::handleWrite " << state_;

    if(state_ == kConnecting){
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        if(err){
            LOG_WARN << "Connector::handleWrite - SO_ERROR = " << err << " " << strerror_tl(err);
            retry(sockfd);
        }else if(sockets::isSelfConnect(sockfd)){
            LOG_WARN << "Connector::handleWrite - Self connect";
            retry(sockfd);
        }else{
            setState(kConnected);
            if(connect_){
                newConnectionCallback_(sockfd);
            }else{
                sockets::close(sockfd);
            }
        }
    }else{
        // what happened?
        assert(state_ == kDisconnected);
    }
}

void Connector::handleError(){
    LOG_ERROR << "Connector::handleError";
    assert(state_ == kConnecting);

    int sockfd = removeAndResetChannel();
    int err = sockets::getSocketError(sockfd);
    LOG_TRACE << "SO_ERROR = " << err << " " << strerror_tl(err);
    retry(sockfd);
}

void Connector::retry(int sockfd){
    sockets::close(sockfd);
    setState(kDisconnected);
    if(connect_){
        LOG_INFO << "Connector::retry - Retry connecting to " << serverAddr_.toHostPort() << " in " << retryDelayMs_ << " milliseconds. ";
        timerId_ = loop_->runAfter(retryDelayMs_/1000.0, boost::bind(&Connector::startInLoop, this)); // FIXME: unsafe
        retryDelayMs_ = std::min(retryDelayMs_*2, kMaxRetryDelayMs);
    }else{
        LOG_DEBUG << "do not connect";
    }
}