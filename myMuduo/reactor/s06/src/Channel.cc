#include "../lib/Channel.h"
#include "../lib/EventLoop.h"
#include "muduo/base/Logging.h"

#include <sstream>

#include <poll.h>

using namespace muduo;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fdArg)
: loop_(loop),
  fd_(fdArg),
  events_(0),
  revents_(0),
  index_(-1){}

Channel::~Channel(){
    assert(!eventHandling_); //防止意外被析构掉  
                             //Prevent accidental destructions
}

void Channel::update(){
    loop_->updateChannel(this);
}

void Channel::handlEvent(){
    eventHandling_ = true;
    if(revents_ & POLLNVAL){
        LOG_WARN << "Channel::handle_event() POLLNVAL";
    }

    if((revents_ & POLLHUP) && !(revents_ & POLLIN)){
        LOG_WARN << "Channel::handle_event() POLLHUP";
        if(closeCallback_) closeCallback_();
    }

    if(revents_ & (POLLIN | POLLPRI | POLLRDHUP)){
        if(readCallback_) readCallback_();
    }

    if(revents_ & POLLOUT){
        if(writeCallback_) writeCallback_();
    }

    eventHandling_ = false;
}