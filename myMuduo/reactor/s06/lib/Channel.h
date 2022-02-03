#ifndef MUDUO_NET_CHANNEL_H
#define MUDUO_NET_CHANNEL_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

namespace muduo{

class EventLoop;

class Channel: boost::noncopyable{
public:
    typedef boost::function<void()> EventCallback;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    void handlEvent();
    void setReadCallback(const EventCallback& cb){ readCallback_ = cb; }
    void setWriteCallback(const EventCallback& cb){ writeCallback_ = cb; }
    void setErrorCallback(const EventCallback& cb){ errorCallback_ = cb; }
    void setCloseCallback(const EventCallback& cb){ closeCallback_ = cb; }

    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revt) { revents_ = revt; }
    bool isNoneEvent() const { return events_ == kNoneEvent; }

    void enableReading() { events_ |= kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    EventLoop* ownerLoop() { return loop_; }

private:
    void update();
    static const int kNoneEvent; //在Channel.cc中定义
    static const int kReadEvent; //在Channel.cc中定义
    static const int kWriteEvent; //在Channel.cc中定义

    EventLoop* loop_;
    const int fd_;
    int events_; //它关心的IO事件
    int revents_; //目前活动的事件
    int index_;

    bool eventHandling_;

    EventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback errorCallback_;
    EventCallback closeCallback_;
};

}

#endif //MUDUO_NET_CHANNEL_H