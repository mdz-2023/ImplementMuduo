#pragma once

#include "noncopyable.h"
#include "Thread.h"
#include <functional>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

class EventLoop;

// 绑定一个loop和thread，thread中创建loop
// one loop pre thread
class EventLoopThread : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;
    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                    const std::string &name = std::string());
    ~EventLoopThread();

    EventLoop *startLoop();

private:
    // 线程函数，在其中创建loop
    void threadFunc(); 

    EventLoop *loop_;
    bool exiting_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
};
