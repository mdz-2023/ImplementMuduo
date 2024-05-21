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
    if (loop == nullptr)
    {
        LOG_FATAL << "subLoop is null!";
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

void TcpConnection::send(const std::string& message)
{
    LOG_DEBUG << "state_:" << stateToString() << "  loop_=" << loop_;
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(message.c_str(), message.size());
        }
        else
        {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop,
                                        this,
                                        message.c_str(),
                                        message.size()));
        }
    }
}

/**
 * 发送数据，应用写得快，而内核发送数据慢，需要将待发送数据写入缓冲区，而且设置水位回调
*/
void TcpConnection::sendInLoop(const void *data, size_t len)
{
    LOG_DEBUG << "TcpConnection::sendInLoop()  send len:" << len;
    ssize_t nwrote = 0;
    size_t remaining = len; // 剩余字节数
    bool faultError = false;
    if(state_ == kDisconnected){
        LOG_ERROR << "disconnected, give up writing";
        return;
    }
    // 刚开始对写事件还不感兴趣
    // 表示channe_第一次开始写数据，而且缓冲区没有待发送的数据，尝试直接发送
    if(!channel_->isWriting() && outputBuffer_.writableBytes() == 0){
        nwrote = ::write(channel_->fd(), data, len); // 返回具体发送个数
        if(nwrote >= 0){
            remaining = len - nwrote;
            if(remaining == 0 && writeCompleteCallback_){ // 全部发送完了，执行用户的写完回调
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else // nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK) // EWOULDBLOCK 表示由于非阻塞没有数据的一个正常返回
            {
                LOG_ERROR << "TcpConnection::sendInLoop";
                if (errno == EPIPE || errno == ECONNRESET) // 接收到对端的重置
                {
                    faultError = true;
                }
            }
        }
    }
    // 说明当前这次write没有把数据全部发送出去，剩余的数据需要保存到缓冲区中
    // 然后给channel注册epollout事件，poller发现tcp的发送缓冲区有空间，会通知相应的sock-channel，
    // 调用writeCallback_回调方法，也就是调用TcpConnction::handleWrite方法，把发送缓冲区中的数据全部发送完成
    if (!faultError && remaining > 0) 
    {
        //  目前发送缓冲区剩余待发送数据长度
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_
            && oldLen < highWaterMark_
            && highWaterMarkCallback_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append(static_cast<const char*>(data)+nwrote, remaining);
        if (!channel_->isWriting())
        {
            // 一定要注册channel的写事件，不然poller不会给channl通知epollout
            channel_->enableWriting();
        }
    }
}

// 用户调用的
void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    if (!channel_->isWriting()) // 说明outputBuffer中的数据已经全部发送完成
    {
        socket_->shutdownWrite(); // 关闭写端，触发epllHUP，调用closeCB，最终执行handleClose
    }
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

void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this()); // Channel绑定本connection，防止TcpConnection对象析构之后Channel还要执行其回调操作
    channel_->enableReading(); // 向poller注册EPOLLIN事件
    // 连接建立，执行回调
    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll(); // 从Epoll中移除所有感兴趣事件

        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
        LOG_DEBUG << "TcpConnection::handleRead()  read bytes from buf:" << n;
        // 已建立连接的用户，有可读事件发生了，调用用户传入的回调onMessage
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

// 有EpollOut事件的时候才执行
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
    channel_->disableAll(); // 在poller上删除Channel所有的感兴趣事件

    TcpConnectionPtr guardThis(shared_from_this());
    if(connectionCallback_){
        connectionCallback_(guardThis); // 执行用户注册的连接回调，用户会在其中判断链接状态
    }
    if(closeCallback_){
        closeCallback_(guardThis); // 关闭连接的回调，调用到TcpServer::removeConnection
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
