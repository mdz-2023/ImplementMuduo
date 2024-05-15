#pragma once
#include "noncopyable.h"
#include "Timestamp.h"
#include <functional>
#include <memory>

// 在cpp文件夹中实际include，可以更少暴露引用文件细节
class EventLoop;

/*
理清楚 EventLoop、Channel、Poller之间的关系  《= Reactor模型上对应 Demultiplex
Channel 理解为通道，封装了socketfd和其他感兴趣的event，如EPOLLIN、EPOLLOUT等
还绑定了poller返回的具体事件
*/

class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;
    Channel(EventLoop *loop, int fd__);
    ~Channel();

    // fd得到poller通知以后，处理事件的 
    void handleEvent(Timestamp recieveTime);
    
    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); } // 使用move将形参的值转移给成员变量
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 防止Channel被手动remove之后，Channel 还在执行回调操作
    void tie(const std::shared_ptr<void>&);

    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revt) { revents_ = revt; } // used by pollers

    // 设置fd相应的事件状态
    void enableReading() { events_ |= kReadEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }

    // 返回fd当前的事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    // for Poller
    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    // one loop per thread, one loop has a poller, one poller can listen many channel
    EventLoop* ownerLoop() { return loop_; }
    void remove(); 
private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);
    static const int kNoneEvent; // 没有事件
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_; // 事件循环
    const int fd_;    // Poller监听的对象
    int events_;      // 注册fd感兴趣的事件
    int revents_;     // it's the received event types of epoll or poll // poller返回的具体发生的事件
    int index_;       // used by Poller.

    std::weak_ptr<void> tie_; // 多线程中监听对象的生存情况
    bool tied_;

    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};
