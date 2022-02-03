#include "../lib/TcpServer.h"

#include "../lib/muduo/base/Logging.h"
#include "../lib/Acceptor.h"
#include "../lib/EventLoop.h"
#include "../lib/SocketsOps.h"
#include "../lib/EventLoopThreadPool.h"
#include <boost/bind.hpp>
#include <stdio.h> // snprintf

using namespace muduo;

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr)
 : loop_(CHECK_NOTNULL(loop)),
   name_(listenAddr.toHostPort()),
   acceptor_(new Acceptor(loop, listenAddr)),
   threadPool_(new EventLoopThreadPool(loop)),
   started_(false),
   nextConnId_(1)
{
    acceptor_->setNewConnectionCallback(boost::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer(){}

void TcpServer::setThreadNum(int numThreads){
    assert(0 <= numThreads);
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start(){
    if(!started_){
        started_ = true;
        threadPool_->start();
    }
    if(!acceptor_->listenning()){
        loop_->runInLoop(boost::bind(&Acceptor::listen, get_pointer(acceptor_))); //get_pointer()函数为获得acceptpr对象的指针
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr){
    loop_->assertInLoopThread();
    char buf[32];
    snprintf(buf, sizeof buf, "#%d", nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO << "TcpServer::newConnection [" << name_ << "] - new connection [" << connName << "] from " << peerAddr.toHostPort();
    InetAddress localAddr(sockets::getLocalAddr(sockfd));
    // FIXME poll with zero timeout to double confirm the new connection
    EventLoop* ioLoop = threadPool_->getNextLoop();
    TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(boost::bind(&TcpServer::removeConnection, this, _1));
    ioLoop->runInLoop(boost::bind(&TcpConnection::connectEstablished, conn));// conn->connectEstablished();
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn){
    // FIXME: unsafe
    loop_->runInLoop(boost::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn){
    loop_->assertInLoopThread();
    LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_ << "] - connection " << conn->name();
    size_t n = connections_.erase(conn->name());
    assert(n == 1); (void)n;
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(boost::bind(&TcpConnection::connectDestoryed, conn));
}