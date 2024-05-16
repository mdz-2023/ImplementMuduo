#pragma once
#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

class Channel;
class Poller;

/**
 * 事件循环类  两大模型：Channel  Poller
 * mainLoop只负责处理IO，并返回client的fd
 * subLoop负责监听poll，并处理相应的回调
 * 两者之间通过weakupfd进行通信
*/
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;
    EventLoop();
    ~EventLoop();

    // 开启loop
    void loop();
    // 退出loop
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    // 在当前loop执行cb
    void runInLoop(Functor cb);
    // 把cb放入队列，唤醒loop所在的线程，执行cb
    void queueInLoop(Functor cb);

    size_t queueSize() const;

    // internal usage
    // 唤醒loop所在的线程
    void wakeup();

    // EventLoop方法 =》 Poller的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断EventLoop对象是否在自己的线程中
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

private:
    // waked up后的一个操作 
    void handleRead();       
    // 执行回调
    void doPendingFunctors(); 

    using ChannelList = std::vector<Channel *>;
    std::atomic_bool looping_; // 原子操作，通过CAS实现
    std::atomic_bool quit_;    // 标识退出loop循环

    const pid_t threadId_; // 记录当前loop所属的线程id

    Timestamp pollReturnTime_; // poller返回发生事件的channels的时间点
    std::unique_ptr<Poller> poller_;

    int wakeupFd_; // 当mainLoop获取一个新用户的channel，通过轮询算法选择一个subloop，通过该成员唤醒subloop处理channel。使用eventfd
    // unlike in TimerQueue, which is an internal class,
    // we don't expose Channel to client.
    std::unique_ptr<Channel> wakeupChannel_;

    // scratch variables
    ChannelList activeChannels_;

    std::atomic_bool callingPendingFunctors_; // 标识当前loop是否有需要执行的回调操作，正在执行则为true
    std::vector<Functor> pendingFunctors_;    // 存储loop需要执行的所有回调操作
    std::mutex mutex_;                        // 保护pendingFunctors_线程安全
};