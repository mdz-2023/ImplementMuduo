#include "TcpConnection.h"
#include "LogStream.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <functional>
#include <sys/types.h> 
#include <sys/socket.h>

static EventLoop *checkLoopNotNull(EventLoop *loop)
{
    if (loop = nullptr)
    {
        LOG_FATAL << "mainLoop is null!";
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop, const std::string &nameArg, int sockfd,
                             const InetAddress &localAddr, const InetAddress &peerAddr)
    : loop_(checkLoopNotNull(loop)),
      name_(nameArg),
      state_(kConnecting),
      reading_(true),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024) // 64M
{
    // 给channel设置相应的回调函数
    // poller给channel通知感兴趣的事件发生了，channel会回调相应的操作函数
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this));

    LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this
              << " fd=" << sockfd;
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_DEBUG << "TcpConnection::dtor[" << name_ << "] at " << this
              << " fd=" << channel_->fd()
              << " state=" << stateToString();
}

const char *TcpConnection::stateToString() const
{
    switch (state_)
    {
    case kDisconnected:
        return "kDisconnected";
    case kConnecting:
        return "kConnecting";
    case kConnected:
        return "kConnected";
    case kDisconnecting:
        return "kDisconnecting";
    default:
        return "unknown state";
    }
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
        // 已建立连接的用户，有可读时间发生了，调用用户传入的回调onMessage
        // 从本对象产生一个智能指针
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_ERROR << "TcpConnection::handleRead";
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (channel_->isWriting())
    {
        int saveError = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &saveError);
        if (n > 0)
        {
            // n个数据写入成功，恢复n个字节
            outputBuffer_.retrieve(n);
            // ==0：已经发送完成了；!=0：还没发送完成
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting(); // ??
                if (writeCompleteCallback_)
                {
                    // 唤醒subLoop对应的线程，在其中执行回调函数
                    // 其实调用 TcpConnection::handleWrite() 函数的时候，已经在这个subLoop中了
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                // 可能client在接收到数据后就会shutdown
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR << "TcpConnection::handleWrite";
        }
    }
    // 上一轮监听中，已经设置了对写事件的不感兴趣
    else
    {
        LOG_INFO << "Connection fd = " << channel_->fd()
                << " is down, no more writing";
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO << "fd = " << channel_->fd() << " state = " << stateToString();
    // we don't close fd, leave it to dtor, so we can find leaks easily.
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    if(connectionCallback_){
        connectionCallback_(guardThis); // 执行用户注册的连接回调，用户会在其中判断链接状态
    }
    if(closeCallback_){
        closeCallback_(guardThis); // 关闭连接的回调
    } 
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    if(::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0){
        LOG_ERROR << "TcpConnection::handleError [" << name_
            << "] - errno = " << errno;
    }
}

void TcpConnection::sendInLoop(const void *message, size_t len)
{
}

void TcpConnection::shutdownInLoop()
{
}
