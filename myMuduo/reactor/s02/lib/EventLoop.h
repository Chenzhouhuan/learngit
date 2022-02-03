#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include "muduo/base/Thread.h"
#include "muduo/base/Timestamp.h"
#include "Callbacks.h"
#include "TimerId.h"
#include "TimerQueue.h"

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


  //Time when poll returns, usually means data arrivial.
  Timestamp pollReturnTime() const { return pollReturnTime_; }
  ///
  /// Runs callback at 'time'.
  ///
  TimerId runAt(const Timestamp& time, const TimerCallback& cb);
  ///
  /// Runs callback after @c delay seconds.
  ///
  TimerId runAfter(double delay, const TimerCallback& cb);
  ///
  /// Runs callback every @c interval seconds.
  ///
  TimerId runEvery(double interval, const TimerCallback& cb);

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
  Timestamp pollReturnTime_;
  boost::scoped_ptr<Poller> poller_;
  boost::scoped_ptr<TimerQueue> timerQueue_;
  ChannelList activeChannels_;
};

}

#endif  // MUDUO_NET_EVENTLOOP_H