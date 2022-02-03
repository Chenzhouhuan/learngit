#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include "muduo/base/Thread.h"
#include "muduo/base/Timestamp.h"
#include "muduo/base/Mutex.h"
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
 typedef boost::function<void()> Functor;

  EventLoop();
  ~EventLoop();

  void loop();

  void quit();

  //Time when poll returns, usually means data arrivial.
  Timestamp pollReturnTime() const { return pollReturnTime_; }

  /// Runs callback immediately in the loop thread.
  /// It wakes up the loop, and run the cb.
  /// If in the same loop thread, cb is run within the function.
  /// Safe to call from other threads.
  void runInLoop(const Functor& cb);
  /// Queues callback in the loop thread.
  /// Runs after finish pooling.
  /// Safe to call from other threads.
  void queueInLoop(const Functor& cb);

  //timers

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

  void cancel(TimerId timerId);

  //internal use only
  void wakeup();
  void updateChannel(Channel* channel);
  void removeChannel(Channel* channel);

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
  void handleRead(); //waked up
  void doPendingFunctors();

  typedef std::vector<Channel*> ChannelList;

  bool looping_; /* atomic */
  bool quit_;
  bool callingPendingFunctors_; /* atomic */
  const pid_t threadId_;
  Timestamp pollReturnTime_;
  boost::scoped_ptr<Poller> poller_;
  boost::scoped_ptr<TimerQueue> timerQueue_;
  int wakeupFd_;
  // unlike in TimerQueue, which is an internal class,
  // we don't expose Channel to client.
  boost::scoped_ptr<Channel> wakeupChannel_;
  ChannelList activeChannels_;
  MutexLock mutex_;
  std::vector<Functor> pendingFunctors_; //@GuardBy mutex_
};

}

#endif  // MUDUO_NET_EVENTLOOP_H