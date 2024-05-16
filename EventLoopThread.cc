#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                 const std::string &name)
    : loop_(nullptr)
    , exiting_(false)
    , thread_(std::bind(&EventLoopThread::threadFunc, this), name)
    , mutex_()
    , cond_()
    , callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();
        thread_.join(); // ?
    }
}

EventLoop *EventLoopThread::startLoop()
{
    thread_.start(); // 启动底层新线程，启动threadFunc

    EventLoop *loop = nullptr; // 此时还未在新线程生成新loop
    {
        std::unique_lock<std::mutex> ulock(mutex_);
        while (loop_ == nullptr)
        {
            cond_.wait(ulock);
        }
        loop = loop_; 
    }
    return loop;
}

// 本方法在新线程中执行
void EventLoopThread::threadFunc()
{
    // 新线程中创建一个独立的EventLoop，和thread一一对应
    // one loop pre thread
    EventLoop loop; 

    if (callback_)
    {
        // 执行上级准备的回调
        callback_(&loop);
    }
    {
        std::unique_lock<std::mutex> ulock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop();// EventLoop *EventLoopThread::loop() => Poller.poll  开始监听、处理读写
    //当前线程（是新线程）进入阻塞状态

    // loop()返回之后：
    std::unique_lock<std::mutex> ulock(mutex_);
    loop_ = nullptr;
}
