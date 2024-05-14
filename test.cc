#include "logger.h"
#include "timestamp.h"
#include "InetAddress.h"
#include "Channel.h"
#include "EPollPoller.h"
#include "EventLoop.h"

int main(){
    // Timestamp t(100);
    // Logger* lo = Logger::getInstance();
    // Logger::LOG_INFO("has some error~~~");

    // InetAddress addr(8080);
    // std::string s = addr.toIpPort();
    // Logger::LOG_INFO(s);

    // Channel cnl(nullptr, 10);
    // cnl.handleEvent(t);

    EventLoop* loop = new EventLoop();
    EPollPoller epoll(loop);
}