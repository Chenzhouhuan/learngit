#include "../lib/EventLoop.h"

#include "../lib/EventLoopThread.h"

#include <boost/bind.hpp>

using namespace muduo;

EventLoopThread::EventLoopThread()
 : loop_(NULL),
   exiting_(false),
   thread_(boost::bind(&EventLoopThread::threadFunc, this)),
   mutex_(),
   cond_(mutex_){}

EventLoopThread::~EventLoopThread(){
    exiting_ = true;
    loop_->quit();
    thread_.join();
}

EventLoop* EventLoopThread::startLoop(){
    assert(!thread_.started());
    thread_.start();

    {
        MutexLockGuard lock(mutex_);
        while(loop_ == NULL){
            //不满足条件时wait，把mutexlock中的锁移除，同时该线程释放该锁
            //Wait to remove the lock from mutexLock and release the lock  
            cond_.wait();
        }
    }

    return loop_;
}

void EventLoopThread::threadFunc(){
    EventLoop loop;

    {
        MutexLockGuard lock(mutex_);
        loop_ = &loop;
        cond_.notify();
    }

    loop.loop();
}