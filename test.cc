#include "logger.h"
#include "Timestamp.h"
#include "InetAddress.h"
#include "Channel.h"
#include "EPollPoller.h"
#include "EventLoop.h"
#include <thread>

int main(){
    // Timestamp t(100);
    // Logger* lo = Logger::getInstance();
    // Logger::LOG_INFO("has some error~~~");

    // InetAddress addr(8080);
    // std::string s = addr.toIpPort();
    // Logger::LOG_INFO(s);

    // Channel cnl(nullptr, 10);
    // cnl.handleEvent(t);

    // EventLoop* loop = new EventLoop();
    // EPollPoller epoll(loop);
    // epoll.newDefaultPoller(loop);

    // std::thread t1([](){
    //     static int i = 0;
    //     while(1)
    //     {
    //         LOG_DEBUG << "in thread1, i = " << i++;
    //     }
    //     while (1)
    //     {
    //         /* code */
    //     }
        
    // });
    // int i = 0;
    // while(1)
    // {
    //     LOG_DEBUG << "in thread0, i = " << i++;
    // }
}