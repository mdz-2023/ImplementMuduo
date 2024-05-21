#include "Channel.h"
#include "EventLoop.h"
#include "LogStream.h"
#include <poll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI; // pool.h
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop *loop, int fd__)
    : loop_(loop),
      fd_(fd__),
      events_(0),
      revents_(0),
      index_(-1),
      tied_(false)
{
}

Channel::~Channel()
{
}

// 在一个TcpConnection新连接建立的时候调用
// TcpConnection 含有一个 Channel，在Channel中保留对其上层对象的指针，
// 防止TcpConnection被用户析构后，本类回调其回调函数
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj; //
    tied_ = true;
    LOG_DEBUG << "Channel::tie() tied_ = true";
}

/*
当改变Channel所表示的fd的events事件后，update负责在poller里更改fd相应的事件  epoll_ctl
EventLoop 有 ChannelList 和 Poller
*/
void Channel::update()
{
    // 通过Channel所属的EventLoop，调用poller的相应方法，注册fd的events事件
    loop_->updateChannel(this);
}

// 在Channel所属的EventLoop中删除当前Channel
void Channel::remove()
{
    loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
    LOG_INFO << "channel handleEvent revents:" << revents_ << "  " << receiveTime.now().toString();
    if (tied_) // 上层TcpConnection对象还在
    {
        std::shared_ptr<void> guard = tie_.lock(); // lock 将弱智能指针 提升为 强智能指针
        if (guard)                                 // 是否存活
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

// 根据poller通知的Channel发生的具体事件，由Channel负责调用具体的回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO << "channel handleEventWithGuard revents:" << revents_ << "  " << receiveTime.now().toString();
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
    {
        if (closeCallback_)
            closeCallback_();
    }

    if (revents_ & POLLERR)
    {
        if (errorCallback_)
            errorCallback_();
    }
    if (revents_ & (POLLIN | POLLPRI))
    {
        if (readCallback_)
            readCallback_(receiveTime);
    }
    if (revents_ & POLLOUT)
    {
        if (writeCallback_)
            writeCallback_();
    }
}
