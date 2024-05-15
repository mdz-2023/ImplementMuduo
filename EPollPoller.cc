#include "EPollPoller.h"
#include "LogStream.h"
#include "EventLoop.h"
#include "Channel.h"
#include "Timestamp.h"
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sstream>

const int kNew = -1;    // 未添加到poller
const int kAdded = 1;   // 已添加
const int kDeleted = 2; // channel已经从poller中删除

// epoll_create
EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)), // epoll_create1 与 epoll_create 不同. EPOLL_CLOEXEC 表示子进程不继承这个fd资源
      events_(kInitEventListSize)
{
    LOG_DEBUG << "epoll_create success:" << 101;
    if (epollfd_ < 0)
    {
        LOG_DEBUG << "epoll_create error:";
    }
}

// epoll fd close
EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

// 通过epoll_wait监听发生事件的fd，将发生事件的channel通过activeChannels地址传回给EventLoop
Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    LOG_DEBUG << "fd total count " << channels_.size(); // 避免实际使用时info打印的过多

    // LT模式，未处理完的会持续上报
    int numEvents = ::epoll_wait(epollfd_,                         // epollfd
                                 &*events_.begin(),                // 事件数组的起始地址：首元素的内存地址，准备存放发生的事件
                                 static_cast<int>(events_.size()), // C++类型安全的强转
                                 timeoutMs);
    int savedErrno = errno; // 记录全局的errno
    Timestamp now(Timestamp::now());
    if (numEvents > 0) // 发生事件的个数
    {
        LOG_DEBUG << numEvents + " events happened";
        fillActiveChannels(numEvents, activeChannels);
        if (numEvents == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        LOG_DEBUG << "nothing happened";
    }
    else
    {
        // error happens, log uncommon ones
        if (savedErrno != EINTR) // != 外部中断
        {
            errno = savedErrno;
            LOG_ERROR << "EPollPoller::poll() err!";
        }
    }
    return now;
}

/**
 *           EventLoop  =》 poller.poll
 *  ChannelList                          Poller
 * （channel不论是否注册）               ChannelMap<fd, channel*> （channel都注册过）
 */
// epoll_ctl    在poller中更新/修改/删除channel
void EPollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index(); // 获取本channel的状态
    LOG_INFO << "fd = " << channel->fd()
             << " events = " << channel->events() << " index = " << index;

    if (index == kNew || index == kDeleted)
    {
        // a new one, add with EPOLL_CTL_ADD
        int fd = channel->fd();
        if (index == kNew)
        {
            channels_[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else
    {
        // update existing one with EPOLL_CTL_MOD/DEL
        int fd = channel->fd();
        if (channel->isNoneEvent()) // 对任何事件都不感兴趣，不需要再监听
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// epoll_ctl    从poller中删除channel
void EPollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    channels_.erase(fd); // 从map中删除
    LOG_INFO << "fd = " << fd;

    int index = channel->index();
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel); // 从Epoll中删除
    }
    channel->set_index(kNew);
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for (int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel); // EventLoop可以拿到所有发生事件的fd
    }
}

// add/mod/del
void EPollPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    memset(&event, 0, sizeof event);
    int fd = channel->fd();
    event.events = channel->events(); // 包含fd的事件
    event.data.fd = fd;
    event.data.ptr = channel; // .data携带额外数据，ptr针对fd携带的数据

    LOG_INFO << "epoll_ctl op = " << operation << " fd = " << fd;

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) // 出错了
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_FATAL << "epoll_ctl op =" << operation << " fd =" << fd;
        }
        else
        {
            LOG_FATAL << "epoll_ctl op =" << operation << " fd =" << fd;
        }
    }
}
