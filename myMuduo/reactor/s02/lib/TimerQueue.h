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

class TimerQueue: boost::noncopyable{
public:
    TimerQueue(EventLoop* loop);
    ~TimerQueue();

    TimerId addTimer(const TimerCallback& cb, Timestamp when, double interval);

private:
    typedef std::pair<Timestamp, Timer*> Entry;
    typedef std::set<Entry> TimerList;

    void handleRead();

    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired, Timestamp now);

    bool insert(Timer* timer);

    EventLoop* loop_;
    const int timerfd_;
    Channel timerfdChannel_;
    //Timer list sorted by expiration
    TimerList timers_;
};

}
#endif //MUDUO_NET_TIMERQUEUE_H