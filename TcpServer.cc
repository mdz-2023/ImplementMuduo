#include "TcpServer.h"

#include <sys/socket.h>

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg, Option option)
    : loop_(loop),
      ipPort_(listenAddr.toIpPort()),
      name_(nameArg),
      acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
      threadPool_(new EventLoopThreadPool(loop, name_)),
      // connectionCallback_(defaultConnectionCallback),
      // messageCallback_(defaultMessageCallback),
      nextConnId_(1)
{
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
    if(started_ == 0){
        started_ = 1;
        threadPool_->start(threadInitCallback_);

        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    EventLoop* ioLoop = threadPool_->getNextLoop();

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
