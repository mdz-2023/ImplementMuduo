#include "EventLoop.h"
#include "LogStream.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>

// 防止一个线程创建多个EventLoop    threadLocal
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义Poller超时时间
const int kPollTimeMs = 10000;

// 创建weakupfd，用来notify唤醒subReactor处理新来的channel
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL << "Failed in eventfd" << errno;
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      callingPendingFunctors_(false),
      threadId_(CurrentThread::tid()),
      poller_(Poller::newDefaultPoller(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG << "EventLoop created " << this << " in thread " << threadId_;
    if (t_loopInThisThread)
    {
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread
                  << " exists in this thread " << threadId_;
    }
    else
    {
        t_loopInThisThread = this;
    }

    // 设置weakupfd的事件类型以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // we are always reading the wakeupfd
    // 每一个EventLoop都将监听weakupChannel的EPOLLIN读事件了
    // 作用是subloop在阻塞时能够被mainLoop通过weakupfd唤醒
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_
              << " destructs in thread " << CurrentThread::tid();
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = NULL;
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;
    LOG_INFO << "EventLoop " << this << " start looping";

    while (!quit_)
    {
        activeChannels_.clear();
        LOG_DEBUG;
        // 当前EventLoop的Poll，监听两类fd，client的fd（正常通信的，在baseloop中）和 weakupfd（mainLoop 和 subLoop 通信用来唤醒sub的）
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        LOG_DEBUG;
        for (Channel *channel : activeChannels_)
        {
            // Poller监听哪些channel发生事件了，然后上报给EventLoop，通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        LOG_DEBUG;
        // 执行当前EventLoop事件循环需要处理的回调操作
        /**
         * IO线程 mainLoop 只 accept 然后返回client通信用的fd <= 用channel打包 并分发给 subloop
         * mainLoop事先注册一个回调cb（需要subLoop来执行），weakup subloop后，
         * 执行下面的方法，执行之前mainLoop注册的cb操作（一个或多个）
         */
        doPendingFunctors();
        LOG_DEBUG;
    }

    LOG_INFO << "EventLoop " << this << " stop looping";
    looping_ = false;
}

/**
 * 退出事件循环
 * 1、loop在自己的线程中 调用quit，此时肯定没有阻塞在poll中
 * 2、在其他线程中调用quit，如在subloop（woker）中调用mainLoop（IO）的qiut
 *
 *                  mainLoop
 * 
 *      Muduo库没有 生产者-消费者线程安全的队列 存储Channel
 *      直接使用wakeupfd进行线程间的唤醒       
 *
 * subLoop1         subLoop2        subLoop3
 */
void EventLoop::quit()
{
    quit_ = true;
    // 2中，此时，若当前woker线程不等于mainLoop线程，将本线程在poll中唤醒
    if (!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb)
{
    // LOG_DEBUG<<"EventLoop::runInLoop  cb:" << (cb != 0);
    if (isInLoopThread()) // 产生段错误
    { // 在当前loop线程中 执行cb
        LOG_DEBUG << "在当前loop线程中 执行cb";
        cb();
    }
    else
    { // 在其他loop线程执行cb，需要唤醒其loop所在线程，执行cb
        LOG_DEBUG << "在其他loop线程执行cb，需要唤醒其loop所在线程，执行cb";
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> ulock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    // 唤醒相应的，需要执行上面回调操作的loop线程
    // 若当前线程正在执行回调doPendingFunctors，但是又有了新的回调cb
    // 防止执行完回调后又阻塞在poll上无法执行新cb，所以预先wakeup写入一个数据
    if (!isInLoopThread() || callingPendingFunctors_) 
    {
        wakeup(); // 唤醒loop所在线程
    }
}

// 用来唤醒loop所在的线程，向wakeupfd写一个数据，wakeupChannel就发生读事件，当前loop线程就会被唤醒
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::updateChannel(Channel *channel)
{
    // channel是发起方，通过loop调用poll
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    // channel是发起方，通过loop调用poll
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}

// 执行回调，由TcpServer提供的回调函数
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true; // 正在执行回调操作

    { // 使用swap，将原pendingFunctors_置空并且释放，其他线程不会因为pendingFunctors_阻塞
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor &functor : functors)
    {
        functor(); // 执行当前loop需要的回调操作
    }

    callingPendingFunctors_ = false;
}
