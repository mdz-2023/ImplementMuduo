#include "Sokect.h"
#include "InetAddress.h"
#include "LogStream.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "netinet/tcp.h"

Sokect::~Sokect()
{
    ::close(sockfd_);
}

void Sokect::bindAddress(const InetAddress &localaddr)
{
    if (0 != ::bind(sockfd_, (sockaddr *)localaddr.getSockAddr(), sizeof(sockaddr_in)))
    {
        LOG_FATAL << "Sokect::bindAddress error, errno: " << errno;
    }
}

void Sokect::listen()
{
    // 定义了一个 “等待连接队列” 的最大长度为1024
    // 队列用于存储那些已经到达服务器但尚未被 accept() 系统调用处理的连接请求
    if (0 != ::listen(sockfd_, 1024))
    {
        LOG_FATAL << "Sokect::listen error, errno: " << errno;
    }
}

int Sokect::accept(InetAddress *peeraddr)
{
    sockaddr_in addr;
    socklen_t len;
    memset(&addr, 0, sizeof addr);
    int connfd = ::accept(sockfd_, (sockaddr *)&addr, &len);
    if (connfd >= 0)
    {
        peeraddr->setSockAddrInet(addr);
    }
    return 0;
}

void Sokect::shutdownWrite()
{
    if (::shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR << "Sokect::shutdownWrite() error, errno: " << errno;
    }
}

void Sokect::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,
                 &optval, static_cast<socklen_t>(sizeof optval));
}

void Sokect::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
                 &optval, static_cast<socklen_t>(sizeof optval));
}

void Sokect::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT,
                           &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0 && on)
    {
        LOG_ERROR << "SO_REUSEPORT failed.";
    }
}

void Sokect::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE,
                 &optval, static_cast<socklen_t>(sizeof optval));
}
