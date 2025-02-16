#include "Poller.h"
#include "EPollPoller.h"
#include <stdlib.h>

// 解耦 PollPoller 和 EPollPoller

Poller* Poller::newDefaultPoller(EventLoop *loop)
{
    if (::getenv("MUDUO_USE_POLL"))
    {
        // return new PollPoller(loop);
    }
    else
    {
        return new EPollPoller(loop);
    }
    return nullptr; 
}