#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include "muduo/base/Thread.h"
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <vector>

namespace muduo
{

class Channel;
class Poller;

class EventLoop : boost::noncopyable
{
 public:

  EventLoop();
  ~EventLoop();

  void loop();

  void quit();

  void updateChannel(Channel* channel);

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

  typedef std::vector<Channel*> ChannelList;

  bool looping_; /* atomic */
  bool quit_;
  const pid_t threadId_;
  boost::scoped_ptr<Poller> poller_;
  ChannelList activeChannels_;
};

}

#endif  // MUDUO_NET_EVENTLOOP_H