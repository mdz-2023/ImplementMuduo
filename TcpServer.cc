#include "TcpServer.h"

#include <sys/socket.h>

static EventLoop* checkLoopNotNull(EventLoop* loop){
    if(loop = nullptr){
        LOG_FATAL<<"mainLoop is null!";
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg, Option option)
    : loop_(checkLoopNotNull(loop)),
      ipPort_(listenAddr.toIpPort()),
      name_(nameArg),
      acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)), // create sokect, listen
      threadPool_(new EventLoopThreadPool(loop, name_)),
    //   connectionCallback_(defaultConnectionCallback),
    //   messageCallback_(defaultMess ageCallback),
      nextConnId_(1)
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
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        // conn->getLoop()->runInLoop( std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
    if(started_ == 0){ // 防止一个sever对象被start多次
        started_ = 1;
        threadPool_->start(threadInitCallback_); // 启动底层的线程池

        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get())); // 执行mainLoop
    }
}

// 运行在主线程mainLoop
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    EventLoop* ioLoop = threadPool_->getNextLoop(); // 选择一个subloop（子线程），正在epoll上阻塞，需要queueInLoop

    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    
    std::string connName = name_ + buf;
    LOG_INFO << "TcpServer::newConnection [" << name_
    << "] - new connection [" << connName
    << "] from " << peerAddr.toIpPort();

    InetAddress localAddr();
    sockaddr_in localaddr;
    memset(&localaddr, 0, sizeof localaddr);
    // if(::getsockname(sockfd, (sockaddr*)localaddr, sizeof localaddr)){

    // }
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
}
