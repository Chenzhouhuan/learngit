#define __STDC_LIMIT_MACROS
#include "../lib/TimerQueue.h"

#include "muduo/base/Logging.h"
#include "../lib/EventLoop.h"
#include "../lib/Timer.h"
#include "../lib/TimerId.h"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include <sys/timerfd.h>

namespace muduo{
namespace detail{

int createTimerfd(){ //Get a descriptor for time  //获得一个对时间的描述符
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    
    if(timerfd < 0){
        LOG_SYSFATAL << "Failed in timerfd_create";
    }
    return timerfd;
}

struct timespec howMuchTimeFromNow(Timestamp when){ //还剩下多少时间
    int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();

    if(microseconds < 100) microseconds = 100;

    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<time_t>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    return ts;
}

void readTimerfd(int timerfd, Timestamp now){
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
    if(n != sizeof howmany){
        LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
    }
}

void resetTimerfd(int timerfd, Timestamp expiration){
    struct itimerspec newValue;
    struct itimerspec oldValue;
    bzero(&newValue, sizeof newValue);
    bzero(&oldValue, sizeof oldValue);
    newValue.it_value = howMuchTimeFromNow(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if(ret){
        LOG_SYSERR << "timerfd_settime()";
    }
}

}
}

using namespace muduo;
using namespace muduo::detail;

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(loop, timerfd_),
      timers_(){
          timerfdChannel_.setReadCallback(boost::bind(&TimerQueue::handleRead, this));
          // we are always reading the timerfd, we disarm it with timerfd_settime.
          timerfdChannel_.enableReading();
      }

TimerQueue::~TimerQueue(){
    ::close(timerfd_);
    // do not remove channel, since we're in EventLoop::dtor();
    for(TimerList::iterator it = timers_.begin(); it != timers_.end(); ++it) delete it->second;
}

//修改addTimer使其能够在其他线程中被安全调用
//Modify addTimer so that it can be safely called in other threads
TimerId TimerQueue::addTimer(const TimerCallback& cb, Timestamp when, double interval){
    Timer* timer = new Timer(cb, when, interval);
    loop_->runInLoop(boost::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

void TimerQueue::cancel(TimerId timerId){
    loop_->runInLoop(boost::bind(&TimerQueue::cancelInLoop, this, timerId));
}

void TimerQueue::addTimerInLoop(Timer* timer){
    loop_->assertInLoopThread();
    bool earliestChanged = insert(timer);

    if(earliestChanged) resetTimerfd(timerfd_, timer->expiration());
}
//Change at this point(修改至此)

void TimerQueue::cancelInLoop(TimerId timerId){
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    ActiveTimer timer(timerId.timer_, timerId.sequence_);
    ActiveTimerSet::iterator it = activeTimers_.find(timer);
    if(it != activeTimers_.end()){
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        assert(n == 1); (void)n;
        delete it->first; // FIXME: no delete please
        activeTimers_.erase(it);
    }else if(callingExpiredTimers_){ //为了应对“自注销”这种情况       //因为此时toCancel自注销已经不在timers_和activeTimers_这两个容器中，而在expired中
                                     //To deal with self-deregistration //ToCancel is not in timers_ or activeTimers_ but in expired  
        cancelingTimers_.insert(timer);
    }
    assert(timers_.size() == activeTimers_.size());
}

void TimerQueue::handleRead(){
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now);

    std::vector<Entry> expired = getExpired(now);

    callingExpiredTimers_ = true;
    cancelingTimers_.clear();

    // safe to callback outside critical section
    for(std::vector<Entry>::iterator it = expired.begin(); it != expired.end(); ++it){
        it->second->run();
    }
    callingExpiredTimers_ = false;

    reset(expired, now);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now){
    assert(timers_.size() == activeTimers_.size());
    std::vector<Entry> expired;
    Entry sentry = std::make_pair(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    TimerList::iterator it = timers_.lower_bound(sentry);
    assert(it == timers_.end() || now < it->first);
    std::copy(timers_.begin(), it, back_inserter(expired));
    timers_.erase(timers_.begin(), it);

    BOOST_FOREACH(Entry entry, expired){
        ActiveTimer timer(entry.second, entry.second->sequence());
        size_t n = activeTimers_.erase(timer);
        assert(n == 1); (void)n;
    }

    assert(timers_.size() == activeTimers_.size());
    return expired;
}

void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now){
    Timestamp nextExpired;

    for(std::vector<Entry>::const_iterator it = expired.begin(); it != expired.end(); ++it){
        ActiveTimer timer(it->second, it->second->sequence());
        if(it->second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end()){
            it->second->restart(now);
            insert(it->second);
        }else{
            // FIXME move to a free list
            delete it->second;
        }
    }

    if(!timers_.empty()){
        nextExpired = timers_.begin()->second->expiration();
    }

    if(nextExpired.valid()){
        resetTimerfd(timerfd_, nextExpired);
    }
}

bool TimerQueue::insert(Timer* timer){
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if(it == timers_.end() || when < it->first){
        earliestChanged = true;
    }
    {
        std::pair<TimerList::iterator, bool> result = timers_.insert(Entry(when, timer));
        assert(result.second); (void)result;
    }

    {
        std::pair<ActiveTimerSet::iterator, bool> result = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
        assert(result.second); (void)result;
    }
    
    assert(timers_.size() == activeTimers_.size());
    return earliestChanged;
}