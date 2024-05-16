#include <thread>
#include "noncopyable.h"
#include <functional>
#include <atomic>
#include <string>
#include <memory>

class Thread : noncopyable
{
public:
    // 若函数带返回值，则需要绑定器相关编程
    using ThreadFunc = std::function<void()>;
    // 构造函数不应被用于隐式类型转换
    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();

    void start();

    void join();

    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const std::string &name() const { return name_; }

    static int numCreated() { return numCreated_; }
private:
    void setDefaultName();
    std::shared_ptr<std::thread> pthread_; // 指针控制线程的开始时机
    bool started_;
    bool joined_; // 是普通的线程，主线程等待本线程执行完再继续。当前线程等待新线程执行完再返回
    pid_t tid_;
    ThreadFunc func_;
    std::string name_;

    static std::atomic_int numCreated_;
};