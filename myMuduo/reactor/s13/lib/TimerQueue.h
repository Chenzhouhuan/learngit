#ifndef MUDUO_NET_TIMERQUEUE_H
#define MUDUO_NET_TIMERQUEUE_H

#include <set>
#include <vector>

#include <boost/noncopyable.hpp>

#include "muduo/base/Timestamp.h"
#include "muduo/base/Mutex.h"
#include "../lib/Callbacks.h"
#include "../lib/Channel.h"

namespace muduo{

class EventLoop;
class Timer;
class TimerId;

///
/// A best efforts timer queue.
/// No guarantee that the callback will be on time.
///

class TimerQueue: boost::noncopyable{
public:
    TimerQueue(EventLoop* loop);
    ~TimerQueue();

  ///
  /// Schedules the callback to be run at given time,
  /// repeats if @c interval > 0.0.
  ///
  /// Must be thread safe. Usually be called from other threads.
    TimerId addTimer(const TimerCallback& cb, Timestamp when, double interval);

    void cancel(TimerId timerId);

private:
    typedef std::pair<Timestamp, Timer*> Entry;
    typedef std::set<Entry> TimerList;
    typedef std::pair<Timer*, int64_t> ActiveTimer;
    typedef std::set<ActiveTimer> ActiveTimerSet;

    void addTimerInLoop(Timer* timer);
    void cancelInLoop(TimerId timerId);
    // called when timerfd alarms
    void handleRead();
    // move out all expired timers
    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired, Timestamp now);

    bool insert(Timer* timer);

    EventLoop* loop_;
    const int timerfd_;
    Channel timerfdChannel_;
    //Timer list sorted by expiration
    TimerList timers_; //Sort by expiration time //按照到期时间排序 

    // for cancel()
    bool callingExpiredTimers_; //atomic
    ActiveTimerSet activeTimers_; //Sort by object address //按照对象地址排序
    ActiveTimerSet cancelingTimers_; //Process the case of self-deregistration, the self-deregistration is put into the table, 
                                     //and the activeTimers_ and Timers_ are not put back, avoiding the cycle of self-deregistration  
                                     //处理自注销情况，自注销的放到该表中，不再放回activeTimers_和timers_中，避免了循环自注销
};

}
#endif //MUDUO_NET_TIMERQUEUE_H