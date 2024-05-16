#include "Acceptor.h"
#include "InetAddress.h"
#include "LogStream.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

static int createNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP); // SOCK_CLOEXEC标志子进程不接受父进程
    if (sockfd < 0)
    {
        LOG_FATAL << "sockets::createNonblockingOrDie";
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reusePort)
    : loop_(loop), acceptSocket_(createNonblocking()), acceptChannel_(loop, acceptSocket_.fd()), listenning_(false)
{
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll(); // 取消fd所有的事件状态
    acceptChannel_.remove();
}

void Acceptor::listen()
{
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::handleRead()
{
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr); // 获取到已连接addr
    if (connfd >= 0)
    {
        if (newConnectionCallback_)
        {
            newConnectionCallback_(connfd, peerAddr);
        }
        else
        { // 来了新连接，但是没有对应的回调函数，即无法为这个客户端服务，则直接关闭
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR << "Acceptor::handleRead():acceptSocket_.accept error, errno: " << errno;
        if (errno == EMFILE)
        {
            LOG_ERROR << "Acceptor::handleRead() error, sockfd reached limit!";
        }
    }
}
