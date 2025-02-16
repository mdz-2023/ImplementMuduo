#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"
#include "LogStream.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg)
    : baseLoop_(baseLoop), name_(nameArg), started_(false), numThreads_(0), next_(0)
{
    LOG_DEBUG << "EventLoopThreadPool::EventLoopThreadPool():" << nameArg;
}

EventLoopThreadPool::~EventLoopThreadPool()
{
}

void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
    started_ = true;
    LOG_DEBUG << "EventLoopThreadPool::start(const ThreadInitCallback &cb)" << "  numThreads_ = " << numThreads_ << " cb:" << (cb != 0);
    for (int i = 0; i < numThreads_; ++i)
    {
        std::string name = name_ + std::to_string(i);
        EventLoopThread *t = new EventLoopThread(cb, name);
        threads_.emplace_back(std::unique_ptr<EventLoopThread>(t));
        loops_.emplace_back(t->startLoop());// 底层创建线程，绑定一个新的EventLoop，并返回该loop的地址
    }
    // 整个服务只有一个线程，运行着baseLoop
    if (numThreads_ == 0 && cb)
    {
        LOG_DEBUG;
        cb(baseLoop_);
    }
    // LOG_DEBUG << "end";
}

EventLoop *EventLoopThreadPool::getNextLoop()
{
    EventLoop *loop = baseLoop_;
    if(!loops_.empty()){
        loop = loops_[next_];
        ++next_;
        if(next_ > loops_.size()){
            next_ = 0;
        }
    }
    return loop;
}

EventLoop *EventLoopThreadPool::getLoopForHash(size_t hashCode)
{
    EventLoop* loop = baseLoop_;
    if(!loops_.empty()){
        loop = loops_[hashCode % loops_.size()]; // 循环提供loops中的loop
    }
    return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops()
{
    if (loops_.empty())
    {
        return std::vector<EventLoop *>(1, baseLoop_);
    }
    else
    {
        return loops_;
    }
}
