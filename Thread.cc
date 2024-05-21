#include "Thread.h"
#include "LogStream.h"
#include "CurrentThread.h"
#include <semaphore.h>
#include <iostream>

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false),
      joined_(false),
      tid_(0),
      func_(std::move(func)), // 底层资源直接转移给func_
      name_(name)
{
    setDefaultName();
}
Thread::~Thread()
{
    // 线程不在运行没必要操作
    if(started_ && !joined_){ // join 和 detach 是线程必须的两种模式，必须选择其一
        pthread_->detach(); // 守护线程，主线程结束的时候此线程自动结束
    }
}

// 一个Thread对象就记录了一个新线程的详细信息
void Thread::start()
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);

    // 开启线程
    pthread_ = std::make_shared<std::thread>([&](){
        // 获取线程的tid值
        tid_ = CurrentThread::tid();
        //信号量+1
        sem_post(&sem);
        // 开启一个新线程，专门执行该线程函数
        func_(); 
    });

    // 等待tid_有值
    sem_wait(&sem);
}
void Thread::join()
{
    joined_ = true;
    pthread_->join();
}

void Thread::setDefaultName()
{
    if (name_.empty())
    {
        name_ = "Thread" + std::to_string(++numCreated_);
    }
}

// int main(){
//     Thread t0([&](){
//         std::cout << " hello t0" <<std::endl;
//         LOG_INFO << "this is in t0";
//     });
//     t0.start();
//     LOG_INFO << t0.name();

//     Thread t1([&](){
//         std::cout << " hello t1" <<std::endl;
//         LOG_INFO << "this is in t1";
//     }, "myt1");
//     t1.start();
//     LOG_INFO << t1.name();
// }