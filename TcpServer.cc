#include "TcpServer.h"

#include <sys/socket.h>
#include <string.h>

static EventLoop *checkLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL << "mainLoop is null!";
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg, Option option)
    : loop_(checkLoopNotNull(loop)),
      ipPort_(listenAddr.toIpPort()),
      name_(nameArg),
      acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)), // create sokect, listen
      threadPool_(new EventLoopThreadPool(loop, name_)),
      connectionCallback_(),
      messageCallback_(),
      nextConnId_(1),
      started_(0)
{
    // 当有新用户链接时，将执行TcpServer::newConnection回调：
    // 根据轮训算法选择一个subLoop并唤醒，把当前connfd封装成channel分发给subloop
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this,
                                                  std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    LOG_INFO << "TcpServer::~TcpServer [" << name_ << "] destructing";
    for (auto &item : connections_)
    {
        // 强智能指针不会再指向原来的对象，放手了，出了作用域，可以自动释放资源
        TcpConnectionPtr conn(item.second);
        item.second.reset(); 
        // 销毁连接
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
    LOG_DEBUG << "TcpServer::start() started_ = " << started_ << "loop_=" << loop_;
    if (started_++ == 0)// 防止一个sever对象被start多次
    { 
        threadPool_->start(threadInitCallback_); // 启动底层的线程池

        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get())); // 注册AcceptorChannel到baseLoop
        LOG_DEBUG << "TcpServer::start success!";
    } 
}

// 有新客户端连接，运行在主线程mainLoop的acceptor会执行这个回调操作
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    // 轮询选择一个subloop（子线程），正在epoll上阻塞，需要queueInLoop
    EventLoop *ioLoop = threadPool_->getNextLoop();

    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;

    std::string connName = name_ + buf;
    LOG_INFO << "TcpServer::newConnection [" << name_
             << "] - new connection [" << connName
             << "] from " << peerAddr.toIpPort();

    // 通过sockfd获取其绑定的本机的ip地址和端口信息
    sockaddr_in localaddr;
    memset(&localaddr, 0, sizeof localaddr);
    // ::bzero(&localaddr, sizeof localaddr);
    socklen_t addrlen = sizeof localaddr;
    if (::getsockname(sockfd, (sockaddr *)&localaddr, &addrlen) < 0)
    {
        LOG_ERROR << "sockets::getLocalAddr error, errno=" << errno;
    }
    InetAddress localAddr(localaddr);

    // 根据连接成功的sockfd，创建TcpConnection
    TcpConnectionPtr conn(new TcpConnection(
        ioLoop,
        connName,
        sockfd,
        localAddr,
        peerAddr)
    );
    connections_[connName] = conn;
    // 下面的回调都是用户来设置给TcpServer=》TcpConnection=》Channel注册=》Poller=》notify Channel回调
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

    // 直接调用TcpConnection::connectEstablished
    // 本函数运行在baseLoop，而ioloop在子线程阻塞在epoll_wait，
    // 此时在baseLoop中进行函数调用runInLoop、queueInLoop进而保存到ioloop的成员变量pendingFunctors_，
    // 进而一直在baseLoop中调用到ioloop的weakup函数，向ioloop的wakeupfd中写一个数据
    // 在子线程中的epoll_wait监听到写事件，就可以返回，才能够执行这里的回调函数
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(
        std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
             << "] - connection " << conn->name();
    connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop(); // 获取conn所在的loop 
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}
