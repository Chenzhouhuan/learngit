#include "../lib/Acceptor.h"

#include "../lib/muduo/base/Logging.h"
#include "../lib/EventLoop.h"
#include "../lib/InetAddress.h"
#include "../lib/SocketsOps.h"

#include <boost/bind.hpp>

using namespace muduo;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr)
    : loop_(loop),
      acceptSocket_(sockets::createNonblockingOrDie()),
      acceptChannel_(loop, acceptSocket_.fd()),
      listenning_(false){
          acceptSocket_.setReuseAddr(true);
          acceptSocket_.bindAddress(listenAddr);
          acceptChannel_.setReadCallback(boost::bind(&Acceptor::handlRead, this));
      }

void Acceptor::listen(){
    loop_->assertInLoopThread();
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::handlRead(){
    loop_->assertInLoopThread();
    InetAddress peerAddr(0);
    //FIXME loop until no more
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd >= 0){
        if(newConnectionCallback_){
            newConnectionCallback_(connfd, peerAddr);
        }else{
            sockets::close(connfd);
        }
    }
}