#pragma once

#include "noncopyable.h"
#include <functional>
#include <string>
#include <memory>
#include <vector>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>; // 初始化EventLoopThread之后的回调函数
    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();
    
    void setThreadNum(int numThreads) { numThreads_ = numThreads; }
    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    // valid after calling start()
    /// round-robin
    // 如果在多线程中，baseLoop_默认以轮询的方式分配channel给subloop
    EventLoop *getNextLoop();

    /// with the same hash code, it will always return the same EventLoop
    // 用不到
    EventLoop *getLoopForHash(size_t hashCode);
    // 用不到
    std::vector<EventLoop *> getAllLoops();

    bool started() const
    {
        return started_;
    }

    const std::string &name() const
    {
        return name_;
    }

private:
    EventLoop *baseLoop_; // 父loop
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop *> loops_;
};