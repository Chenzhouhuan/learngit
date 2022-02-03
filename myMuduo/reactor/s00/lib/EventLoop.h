// #ifndef MUDUO_NET_EVENTLOOP_H
// #define MUDUO_NET_EVENTLOOP_H

// #include <muduo/base/Thread.h>
// #include <boost/noncopyable.hpp>

// namespace muduo{

// class EventLoop: boost::noncopyable{
// public:
//     EventLoop();
//     ~EventLoop();

//     void loop();

//     void assertInLoopThread(){
//         if(!isInLoopThread()){
//             abortNotInLoopThread();
//         }
//     }

//     bool isInLoopThread() const{ return threadId_ == CurrentThread::tid(); }

// private:
//     void abortNotInLoopThread();

//     bool looping_;
//     const pid_t threadId_;
    
// };

// }

// #endif  // MUDUO_NET_EVENTLOOP_H

#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include "muduo/base/Thread.h"
#include <boost/noncopyable.hpp>

namespace muduo
{

class EventLoop : boost::noncopyable
{
 public:

  EventLoop();
  ~EventLoop();

  void loop();

  void assertInLoopThread()
  {
    if (!isInLoopThread())
    {
      abortNotInLoopThread();
    }
  }

  bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

 private:

  void abortNotInLoopThread();

  bool looping_; /* atomic */
  const pid_t threadId_;
};

}

#endif  // MUDUO_NET_EVENTLOOP_H